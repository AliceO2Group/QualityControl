// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
#include "QualityControl/QualityObject.h"

namespace o2::quality_control::core
{

const char* noQualityObjectsComment = "No Quality Objects found within the specified time range";

QualitiesToTRFCollectionConverter::QualitiesToTRFCollectionConverter(std::string trfcName, std::string detectorCode, uint64_t startTimeLimit, uint64_t endTimeLimit, std::string qoPath)
  : mStartTimeLimit(startTimeLimit),
    mEndTimeLimit(endTimeLimit),
    mQOPath(qoPath),
    mConverted(new TimeRangeFlagCollection(trfcName, detectorCode)),
    mCurrentStartTime(0),
    mCurrentEndTime(startTimeLimit), // this is to correctly set the missing QO time range if none were given
    mQOsIncluded(0),
    mWorseThanGoodQOs(0)
{
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

  mCurrentStartTime = validFrom < mStartTimeLimit ? mStartTimeLimit : validFrom;
  mCurrentEndTime = validUntil > mEndTimeLimit ? mEndTimeLimit : validUntil;

  if (!mCurrentTRF.has_value() && newQO.getQuality().isWorseThan(Quality::Good)) {
    // There was no TRF in the previous step and the data quality is bad now.
    // We create a new TRF and we will work on it in next loop iterations.
    mCurrentTRF.emplace(mCurrentStartTime, mCurrentEndTime, FlagReasonFactory::Unknown(), newQO.getMetadata("comment", ""), newQO.getPath());

  } else if (mCurrentTRF.has_value()) {
    // There is already a TRF. We will check if it can be merged with the new QO.
    auto newFlag = FlagReasonFactory::Unknown(); // todo: use reasons from QOs
    auto newComment = newQO.getMetadata("comment", "");

    if (newQO.getQuality() == Quality::Good) {
      // The data quality is not bad anymore.
      // We trim the current TRF's time range if necessary and deposit it to the collection.
      if (mCurrentTRF->getEnd() > validFrom) {
        mCurrentTRF->setEnd(validFrom);
      }
      mConverted->insert(mCurrentTRF.value());
      mCurrentTRF.reset();
    } else if (mCurrentTRF->getFlag() != newFlag || mCurrentTRF->getComment() != newComment) {
      // The data quality is still bad, but in a different way.
      // We trim the current TRF's time range if necessary and deposit it to the collection.
      // Then, we create a new TRF.
      if (mCurrentTRF->getEnd() > validFrom) {
        mCurrentTRF->setEnd(validFrom);
      }
      mConverted->insert(mCurrentTRF.value());

      mCurrentTRF.emplace(validFrom, mCurrentEndTime, newFlag, newComment, newQO.getName());
    } else if (mCurrentTRF->getEnd() < mCurrentEndTime) {
      // The data quality is still bad and in the same way.
      // We extend the duration of the current TRF.
      mCurrentTRF->setEnd(mCurrentEndTime);
    }
    // If none of the above conditions was fulfilled,
    // then mCurrentTRF covers larger time range than the new one, keep the old one.
  }
}

std::unique_ptr<TimeRangeFlagCollection> QualitiesToTRFCollectionConverter::getResult()
{
  // handling the end of the time range
  if (mCurrentTRF.has_value()) {
    mConverted->insert(mCurrentTRF.value());
    mCurrentEndTime = mCurrentTRF->getEnd();
  }
  if (mCurrentEndTime < mEndTimeLimit) {
    mConverted->insert({ mCurrentEndTime, mEndTimeLimit, FlagReasonFactory::MissingQualityObject(), noQualityObjectsComment, mQOPath });
  }

  auto result = std::make_unique<TimeRangeFlagCollection>(mConverted->getName(), mConverted->getDetector());
  result.swap(mConverted);

  mCurrentStartTime = 0;
  mCurrentEndTime = mStartTimeLimit;
  mCurrentTRF.reset();
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