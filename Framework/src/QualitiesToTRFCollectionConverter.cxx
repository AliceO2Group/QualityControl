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
/// \file    QualitiesToTRFCollectionConverter.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/QualitiesToTRFCollectionConverter.h"

#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>

#include <utility>
#include "QualityControl/QualityObject.h"

namespace o2::quality_control::core
{

const char* noQualityObjectsComment = "No Quality Objects found within the specified time range";

QualitiesToTRFCollectionConverter::QualitiesToTRFCollectionConverter(std::string trfcName, std::string detectorCode, uint64_t startTimeLimit, uint64_t endTimeLimit, std::string qoPath)
  : mStartTimeLimit(startTimeLimit),
    mEndTimeLimit(endTimeLimit),
    mQOPath(std::move(qoPath)),
    mConverted(new TimeRangeFlagCollection(std::move(trfcName), std::move(detectorCode))),
    mCurrentStartTime(0),
    mCurrentEndTime(startTimeLimit), // this is to correctly set the missing QO time range if none were given
    mQOsIncluded(0),
    mWorseThanGoodQOs(0)
{
}

std::vector<TimeRangeFlag> QO2TRFs(uint64_t startTime, uint64_t endTime, const QualityObject& qo)
{
  auto& reasons = qo.getReasons();
  auto qoPath = qo.getPath();

  if (qo.getQuality().isWorseThan(Quality::Good) && reasons.empty()) {
    return { { startTime, endTime, FlagReasonFactory::Unknown(), {}, qoPath } };
  } else {
    std::vector<TimeRangeFlag> result;
    result.reserve(reasons.size());
    for (const auto& reason : reasons) {
      result.emplace_back(startTime, endTime, reason.first, reason.second, qoPath);
    }
    return result;
  }
}
void QualitiesToTRFCollectionConverter::operator()(const QualityObject& newQO)
{
  if (mConverted->getDetector() != newQO.getDetectorName()) {
    throw std::runtime_error("The TRFCollection '" + mConverted->getName() +
                             "' expects QOs from detector '" + mConverted->getDetector() +
                             "' but received a QO for '" + newQO.getDetectorName() + "'");
  }

  mQOsIncluded++;
  if (newQO.getQuality().isWorseThan(Quality::Good)) {
    mWorseThanGoodQOs++;
  }

  uint64_t validFrom = strtoull(newQO.getMetadata("Valid-From").c_str(), nullptr, 10);
  uint64_t validUntil = strtoull(newQO.getMetadata("Valid-Until").c_str(), nullptr, 10);
  if (validFrom < mCurrentStartTime) {
    throw std::runtime_error("The currently provided QO is dated as earlier than the one before (" //
                             + std::to_string(validFrom) + " vs. " + std::to_string(mCurrentStartTime) +
                             "). QOs should be provided to the QualitiesToTRFCollectionConverter in the chronological order");
  }

  // Is the beginning of time range covered by the first QO provided?
  if (mCurrentStartTime < mStartTimeLimit && validFrom > mStartTimeLimit) {
    mConverted->insert({ mStartTimeLimit, validFrom - 1, FlagReasonFactory::MissingQualityObject(), noQualityObjectsComment, mQOPath });
  }

  mCurrentStartTime = std::max(validFrom, mStartTimeLimit);
  mCurrentEndTime = std::min(validUntil, mEndTimeLimit);

  auto newTRFs = QO2TRFs(mCurrentStartTime, mCurrentEndTime, newQO);

  for (auto& newTRF : newTRFs) {
    auto trfsOverlap = [&newTRF](const TimeRangeFlag& other) {
      return newTRF.getFlag() == other.getFlag() &&
             newTRF.getComment() == other.getComment() &&
             newTRF.getStart() <= other.getEnd() + 1;
    };
    if (auto matchingCurrentTRF = std::find_if(mCurrentTRFs.begin(), mCurrentTRFs.end(), trfsOverlap);
        matchingCurrentTRF != mCurrentTRFs.end()) {
      // we broaden the range in the new one, so it covers also the old range...
      newTRF.getInterval().update(matchingCurrentTRF->getStart());
      newTRF.getInterval().update(matchingCurrentTRF->getEnd());
      // ...and we delete the old one.
      mCurrentTRFs.erase(matchingCurrentTRF);
    }
  }

  // the leftovers are TRFs which are no longer valid.
  for (auto& outdatedTRF : mCurrentTRFs) {
    outdatedTRF.setEnd(std::min(outdatedTRF.getEnd(), mCurrentStartTime));
    // if there is a zero length interval, then a new TRF came which should completely overwrite the outdated one.
    // no need to store it.
    if (!outdatedTRF.getInterval().isZeroLength()) {
      mConverted->insert(outdatedTRF);
    }
  }
  mCurrentTRFs.swap(newTRFs);
}

std::unique_ptr<TimeRangeFlagCollection> QualitiesToTRFCollectionConverter::getResult()
{
  // handling the end of the time range
  for (const auto& trf : mCurrentTRFs) {
    mConverted->insert(trf);
    mCurrentEndTime = std::max(mCurrentEndTime, trf.getEnd());
  }
  if (mCurrentEndTime < mEndTimeLimit) {
    mConverted->insert({ mCurrentEndTime, mEndTimeLimit, FlagReasonFactory::MissingQualityObject(), noQualityObjectsComment, mQOPath });
  }

  auto result = std::make_unique<TimeRangeFlagCollection>(mConverted->getName(), mConverted->getDetector());
  result.swap(mConverted);

  mCurrentStartTime = 0;
  mCurrentEndTime = mStartTimeLimit;
  mCurrentTRFs.clear();
  mQOsIncluded = 0;
  mWorseThanGoodQOs = 0;

  return result;
}

size_t QualitiesToTRFCollectionConverter::getQOsIncluded() const
{
  return mQOsIncluded;
}
size_t QualitiesToTRFCollectionConverter::getWorseThanGoodQOs() const
{
  return mWorseThanGoodQOs;
}

} // namespace o2::quality_control::core
