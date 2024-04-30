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
#include "QualityControl/QualityObject.h"
#include "QualityControl/ObjectMetadataKeys.h"

using namespace o2::quality_control::repository;

namespace o2::quality_control::core
{

const char* noQualityObjectsComment = "No Quality Objects found within the specified time range";

QualitiesToFlagCollectionConverter::QualitiesToFlagCollectionConverter(std::unique_ptr<QualityControlFlagCollection> qcfc, std::string qoPath)
  : mQOPath(std::move(qoPath)),
    mConverted(std::move(qcfc)),
    mCurrentEndTime(mConverted->getStart())
{
}

std::vector<QualityControlFlag> QO2Flags(uint64_t startTime, uint64_t endTime, const QualityObject& qo)
{
  auto& flags = qo.getFlags();
  auto qoPath = qo.getPath();

  if (qo.getQuality().isWorseThan(Quality::Good) && flags.empty()) {
    return { { startTime, endTime, FlagTypeFactory::Unknown(), {}, qoPath } };
  } else {
    std::vector<QualityControlFlag> result;
    result.reserve(flags.size());
    for (const auto& flag : flags) {
      result.emplace_back(startTime, endTime, flag.first, flag.second, qoPath);
    }
    return result;
  }
}

void QualitiesToFlagCollectionConverter::operator()(const QualityObject& newQO)
{
  if (mConverted->getDetector() != newQO.getDetectorName()) {
    throw std::runtime_error("The FlagCollection '" + mConverted->getName() +
                             "' expects QOs from detector '" + mConverted->getDetector() +
                             "' but received a QO for '" + newQO.getDetectorName() + "'");
  }

  mQOsIncluded++;
  if (newQO.getQuality().isWorseThan(Quality::Good)) {
    mWorseThanGoodQOs++;
  }
  // TODO support a scenario when at the beginning of run the data Quality is null, because it could not be judged,
  // but then it evolves to bad or good. Null quality should be probably removed.

  uint64_t validFrom = strtoull(newQO.getMetadata(metadata_keys::validFrom).c_str(), nullptr, 10);
  uint64_t validUntil = strtoull(newQO.getMetadata(metadata_keys::validUntil).c_str(), nullptr, 10);
  if (validFrom < mCurrentStartTime) {
    throw std::runtime_error("The currently provided QO is dated as earlier than the one before (" //
                             + std::to_string(validFrom) + " vs. " + std::to_string(mCurrentStartTime) +
                             "). QOs should be provided to the QualitiesToFlagCollectionConverter in the chronological order");
  }

  // Is the beginning of time range covered by the first QO provided?
  if (mCurrentStartTime < mConverted->getStart() && validFrom > mConverted->getStart()) {
    mConverted->insert({ mConverted->getStart(), validFrom - 1, FlagTypeFactory::UnknownQuality(), noQualityObjectsComment, newQO.getPath() });
  }

  mCurrentStartTime = std::max(validFrom, mConverted->getStart());
  mCurrentEndTime = std::min(validUntil, mConverted->getEnd());

  auto newFlags = QO2Flags(mCurrentStartTime, mCurrentEndTime, newQO);

  for (auto& newFlag : newFlags) {
    auto flagsOverlap = [&newFlag](const QualityControlFlag& other) {
      return newFlag.getFlag() == other.getFlag() &&
             newFlag.getComment() == other.getComment() &&
             newFlag.getStart() <= other.getEnd() + 1;
    };
    if (auto matchingCurrentFlag = std::find_if(mCurrentFlags.begin(), mCurrentFlags.end(), flagsOverlap);
        matchingCurrentFlag != mCurrentFlags.end()) {
      // we broaden the range in the new one, so it covers also the old range...
      newFlag.getInterval().update(matchingCurrentFlag->getStart());
      newFlag.getInterval().update(matchingCurrentFlag->getEnd());
      // ...and we delete the old one.
      mCurrentFlags.erase(matchingCurrentFlag);
    }
  }

  // the leftovers are Flags which are no longer valid.
  for (auto& outdatedFlag : mCurrentFlags) {
    outdatedFlag.setEnd(std::min(outdatedFlag.getEnd(), mCurrentStartTime));
    mConverted->insert(outdatedFlag);
  }
  mCurrentFlags.swap(newFlags);
}

std::unique_ptr<QualityControlFlagCollection> QualitiesToFlagCollectionConverter::getResult()
{
  // handling the end of the time range
  for (const auto& flag : mCurrentFlags) {
    mConverted->insert(flag);
    mCurrentEndTime = std::max(mCurrentEndTime, flag.getEnd());
  }
  if (mCurrentEndTime < mConverted->getEnd()) {
    mConverted->insert({ mCurrentEndTime, mConverted->getEnd(), FlagTypeFactory::UnknownQuality(), noQualityObjectsComment, mQOPath });
  }

  auto result = std::make_unique<QualityControlFlagCollection>(
    mConverted->getName(), mConverted->getDetector(), mConverted->getInterval(),
    mConverted->getRunNumber(), mConverted->getPeriodName(), mConverted->getPassName(), mConverted->getProvenance());
  result.swap(mConverted);

  mCurrentStartTime = 0;
  mCurrentEndTime = mConverted->getStart();
  mCurrentFlags.clear();
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

} // namespace o2::quality_control::core
