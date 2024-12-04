// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    QualitiesToFlagCollectionConverter.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/QualitiesToFlagCollectionConverter.h"

#include <DataFormatsQualityControl/QualityControlFlagCollection.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <utility>
#include <ranges>
#include "fmt/core.h"
#include "QualityControl/QualityObject.h"
// #include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/FlagHelpers.h"
#include "QualityControl/QcInfoLogger.h"

// using namespace o2::quality_control::repository;

namespace o2::quality_control::core
{

const char* noQOComment = "Did not receive a Quality Object which covers this period";
const char* toBeRemovedComment = "This flag should be removed before returning the QCFC";

QualitiesToFlagCollectionConverter::QualitiesToFlagCollectionConverter(
  std::unique_ptr<QualityControlFlagCollection> qcfc, std::string qoPath)
  : mQOPath(std::move(qoPath)),
    mConverted(std::move(qcfc))
{
  if (mConverted == nullptr) {
    throw std::runtime_error("nullptr QualityControlFlagCollection provided to QualitiesToFlagCollectionConverter");
  }
  if (mConverted->size() > 0) {
    throw std::runtime_error(
      "QualityControlFlagCollection provided to QualitiesToFlagCollectionConverter should have no flags");
  }
  if (auto v = mConverted->getInterval(); v.isInvalid()) {
    ILOG(Warning, Support) << fmt::format(
                                "QualityControlFlagCollection provided to QualitiesToFlagCollectionConverter has invalid validity ({}, {})",
                                v.getMin(), v.getMax())
                           << ENDM;
    return;
  }

  /// Timespans not covered by a given QO are filled with Flag 1 (Unknown Quality)
  // This flag will be removed or trimmed by any other Flags received as input.
  mFlagBuffer.insert({ mConverted->getInterval().getMin(), mConverted->getInterval().getMax(),
                       FlagTypeFactory::UnknownQuality(), noQOComment, mQOPath });
}

std::vector<QualityControlFlag> QO2Flags(const QualityObject& qo)
{
  if (qo.getValidity().isInvalid()) {
    return {};
  }

  const auto& flags = qo.getFlags();
  const auto quality = qo.getQuality();
  const auto qoPath = qo.getPath();
  const auto startTime = qo.getValidity().getMin();
  const auto endTime = qo.getValidity().getMax();

  if (!flags.empty()) {
    /// All QOs *with* Flags are converted to Flags, while Quality is ignored.
    std::vector<QualityControlFlag> result;
    result.reserve(flags.size());
    for (const auto& flag : flags) {
      result.emplace_back(startTime, endTime, flag.first, flag.second, qoPath);
    }
    return result;
  }

  if (quality == Quality::Good) {
    /// Good QOs with *no* Flags are not converted to any Flags
    // ...BUT we create a dummy Good flag to mark the timespans that should cancel UnknownQuality flag.
    // We remove these dummy Flags before returning the QCFC. I spent hours thinking if there is
    // a more elegant approach and I could not think of anything better.
    return { { startTime, endTime, FlagTypeFactory::Good(), toBeRemovedComment, qoPath } };
  }
  if (quality.isWorseThan(Quality::Good) && quality.isBetterThan(Quality::Null)) {
    /// Bad and Medium QOs with *no* Flags are converted to Flag 14 (Unknown)
    return { { startTime, endTime, FlagTypeFactory::Unknown(), quality.getName() + " quality with no Flags associated", qoPath } };
  }
  if (quality == Quality::Null) {
    /// Null QOs with *no* Flags are converted to Flag 1 (UnknownQuality)
    return { { startTime, endTime, FlagTypeFactory::UnknownQuality(), quality.getName() + " quality with no Flags associated", qoPath } };
  }
  return {};
}

void QualitiesToFlagCollectionConverter::operator()(const QualityObject& newQO)
{
  if (mConverted->getDetector() != newQO.getDetectorName()) {
    throw std::runtime_error("The FlagCollection '" + mConverted->getName() +
                             "' expects QOs from detector '" + mConverted->getDetector() +
                             "' but received a QO for '" + newQO.getDetectorName() + "'");
  }
  if (mQOPath != newQO.getPath()) {
    throw std::runtime_error("The FlagCollection '" + mConverted->getName() +
                             "' expects QOs for path '" + mQOPath +
                             "' but received a QO for '" + newQO.getPath() + "'");
  }
  if (newQO.getValidity().isInvalid()) {
    ILOG(Warning, Support)
      << fmt::format("Received a QO '{}' with invalid validity interval ({}. {}), ignoring", newQO.GetName(),
                     newQO.getValidity().getMin(), newQO.getValidity().getMax())
      << ENDM;
    return;
  }

  if (mConverted->getInterval().isOutside(newQO.getValidity())) {
    ILOG(Warning, Support) << fmt::format(
                                "The provided QO's validity ({}, {}) is outside of the validity interval accepted by the converter ({}, {})",
                                newQO.getValidity().getMin(), newQO.getValidity().getMax(),
                                mConverted->getInterval().getMin(), mConverted->getInterval().getMax())
                           << ENDM;
    return;
  }

  mQOsIncluded++;
  if (newQO.getQuality().isWorseThan(Quality::Good)) {
    mWorseThanGoodQOs++;
  }

  for (auto&& newFlag : QO2Flags(newQO)) {
    insert(std::move(newFlag));
  }
}

void QualitiesToFlagCollectionConverter::trimBufferWithInterval(ValidityInterval interval, const std::function<bool(const QualityControlFlag&)>& predicate)
{
  auto toTrimPredicate = [&](const QualityControlFlag& flag) {
    return flag_helpers::intervalsOverlap(flag.getInterval(), interval) && predicate(flag);
  };

  auto flagsToTrim = mFlagBuffer | std::views::filter(toTrimPredicate);
  for (const auto& flagToTrimOrRemove : flagsToTrim) {
    auto trimmedFlags = flag_helpers::excludeInterval(flagToTrimOrRemove, interval);
    mFlagBuffer.insert(std::make_move_iterator(trimmedFlags.begin()), std::make_move_iterator(trimmedFlags.end()));
  }
  // we have inserted any trimmed flags which remained, we remove the old ones.
  std::erase_if(mFlagBuffer, toTrimPredicate);
}

std::vector<QualityControlFlag> QualitiesToFlagCollectionConverter::trimFlagAgainstBuffer(const QualityControlFlag& newFlag, const std::function<bool(const QualityControlFlag&)>& predicate)
{
  auto trimmerPredicate = [&](const QualityControlFlag& flag) {
    return flag_helpers::intervalsOverlap(flag.getInterval(), newFlag.getInterval()) && predicate(flag);
  };
  std::vector<QualityControlFlag> trimmedNewFlags{ newFlag };
  for (const auto& overlappingFlag : mFlagBuffer | std::views::filter(trimmerPredicate)) {
    std::vector<QualityControlFlag> updatedTrimmedFlags;
    for (const auto& trimmedFlag : trimmedNewFlags) {
      auto result = flag_helpers::excludeInterval(trimmedFlag, overlappingFlag.getInterval());
      updatedTrimmedFlags.insert(updatedTrimmedFlags.end(), std::make_move_iterator(result.begin()), std::make_move_iterator(result.end()));
    }
    trimmedNewFlags = std::move(updatedTrimmedFlags);
  }

  return trimmedNewFlags;
}

void QualitiesToFlagCollectionConverter::insert(QualityControlFlag&& newFlag)
{
  // We trim the flag to the current QCFC duration
  auto trimmedFlagOptional = flag_helpers::intersection(newFlag, mConverted->getInterval());
  if (!trimmedFlagOptional.has_value()) {
    return;
  }
  newFlag = trimmedFlagOptional.value();

  // We look for any existing flags with could be merged, including cases when there is more than one to be merged.
  // Existing flags: [-----)      [---------)
  // New flag:           [--------)
  // Correct result: [----------------------)
  auto canBeMergedWithNewFlag = [&](const QualityControlFlag& other) {
    return newFlag.getFlag() == other.getFlag() &&
           newFlag.getComment() == other.getComment() &&
           flag_helpers::intervalsConnect(newFlag.getInterval(), other.getInterval());
  };

  auto flagsToMerge = mFlagBuffer | std::views::filter(canBeMergedWithNewFlag);
  for (const auto& flag : flagsToMerge) {
    newFlag.getInterval().update(flag.getStart());
    newFlag.getInterval().update(flag.getEnd());
  }
  std::erase_if(mFlagBuffer, canBeMergedWithNewFlag);

  if (newFlag.getFlag() != FlagTypeFactory::UnknownQuality()) {
    // We trim any UnknownQuality flags which become obsolete due to the presence of the new flag
    trimBufferWithInterval(newFlag.getInterval(),
                           [](const auto& f) { return f.getFlag() == FlagTypeFactory::UnknownQuality(); });
    mFlagBuffer.insert(newFlag);
  } else {
    // If the new Flag is UnknownQuality, we will apply it only for intervals not covered by other types of Flags
    auto trimmedNewFlags = trimFlagAgainstBuffer(newFlag, [](const auto& f) { return f.getFlag() != FlagTypeFactory::UnknownQuality(); });
    mFlagBuffer.insert(std::make_move_iterator(trimmedNewFlags.begin()), std::make_move_iterator(trimmedNewFlags.end()));

    // And then, we trim also the default UnknownQuality flag (no QO).
    if (newFlag.getComment() != noQOComment) {
      trimBufferWithInterval(newFlag.getInterval(), [](const auto& f) {
        return f.getFlag() == FlagTypeFactory::UnknownQuality() && f.getComment() == noQOComment;
      });
    }
  }
}

std::unique_ptr<QualityControlFlagCollection> QualitiesToFlagCollectionConverter::getResult()
{
  std::erase_if(mFlagBuffer, [](const QualityControlFlag& flag) { return flag.getComment() == toBeRemovedComment; });
  for (const auto& flag : mFlagBuffer) {
    mConverted->insert(flag);
  }

  ILOG(Debug, Devel) << fmt::format("converted flags for det '{}' and QO '{}' from {} QOs, incl. {} QOs worse than Good", mConverted->getDetector(), mQOPath, mQOsIncluded, mWorseThanGoodQOs) << ENDM;
  ILOG(Debug, Devel) << *mConverted << ENDM;

  auto result = std::make_unique<QualityControlFlagCollection>(
    mConverted->getName(), mConverted->getDetector(), mConverted->getInterval(),
    mConverted->getRunNumber(), mConverted->getPeriodName(), mConverted->getPassName(), mConverted->getProvenance());
  result.swap(mConverted);

  mFlagBuffer.clear();
  mFlagBuffer.insert({ mConverted->getInterval().getMin(), mConverted->getInterval().getMax(), FlagTypeFactory::UnknownQuality(), noQOComment, mQOPath });
  mQOsIncluded = 0;
  mWorseThanGoodQOs = 0;

  return result;
}

size_t QualitiesToFlagCollectionConverter::getQOsIncluded() const
{
  return mQOsIncluded;
}

size_t QualitiesToFlagCollectionConverter::getWorseThanGoodQOs() const
{
  return mWorseThanGoodQOs;
}

void QualitiesToFlagCollectionConverter::updateValidityInterval(const ValidityInterval interval)
{
  // input validation
  if (interval.isInvalid() || mConverted->getInterval().getOverlap(interval).isZeroLength()) {
    mFlagBuffer.clear();
    mConverted->setInterval(interval);
    return;
  }

  // trimming existing flags
  if (mConverted->getStart() < interval.getMin() || mConverted->getEnd() > interval.getMax()) {
    std::set<QualityControlFlag> trimmedFlags;
    for (const auto& flag : mFlagBuffer) {
      if (auto&& trimmedFlag = flag_helpers::intersection(flag, interval); trimmedFlag.has_value()) {
        trimmedFlags.insert(std::move(trimmedFlag.value()));
      }
    }
    mFlagBuffer.swap(trimmedFlags);
  }

  // adding UnknownQuality to new intervals
  if (mConverted->getStart() > interval.getMin()) {
    QualityControlFlag flag{
      interval.getMin(), mConverted->getStart(), FlagTypeFactory::UnknownQuality(), noQOComment, mQOPath
    };
    mConverted->setStart(interval.getMin());
    insert(std::move(flag));
  }
  if (mConverted->getEnd() < interval.getMax()) {
    QualityControlFlag flag{
      mConverted->getEnd(), interval.getMax(), FlagTypeFactory::UnknownQuality(), noQOComment, mQOPath
    };
    mConverted->setEnd(interval.getMax());
    insert(std::move(flag));
  }
  mConverted->setInterval(interval);
}

int QualitiesToFlagCollectionConverter::getRunNumber() const
{
  return mConverted ? mConverted->getRunNumber() : -1;
}

} // namespace o2::quality_control::core
