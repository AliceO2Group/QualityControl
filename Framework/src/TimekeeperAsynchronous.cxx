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
/// \file   TimekeeperAsynchronous.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TimekeeperAsynchronous.h"
#include "QualityControl/QcInfoLogger.h"

#include <CommonConstants/LHCConstants.h>
#include <Framework/TimingInfo.h>

namespace o2::quality_control::core
{

TimekeeperAsynchronous::TimekeeperAsynchronous(validity_time_t windowLengthMs)
  : Timekeeper(), mWindowLengthMs(windowLengthMs)
{
}

void TimekeeperAsynchronous::updateByCurrentTimestamp(validity_time_t timestampMs)
{
  // async QC should ignore current timestamp
}

void TimekeeperAsynchronous::updateByTimeFrameID(uint32_t tfid)
{
  // fixme: We might want to use this once we know how to get orbitResetTime:
  //  std::ceil((timingInfo.firstTForbit * o2::constants::lhc::LHCOrbitNS / 1000 + orbitResetTime) / 1000);
  //  Until then, we use a less precise method:
  if (mActivityDuration.isInvalid()) {
    ILOG(Warning, Support)
      << "trying to update the validity range with TF ID without having set the activity duration, returning" << ENDM;
    return;
  }
  if (tfid == 0) {
    if (!mWarnedAboutTfIdZero) {
      ILOG(Warning, Support) << "Seen TFID equal to 0, which is not expected in production data. Will use 1 instead, will not warn further." << ENDM;
      mWarnedAboutTfIdZero = true;
      tfid = 1;
    }
  }

  auto tfValidity = computeTimestampFromTimeframeID(tfid);
  mCurrentSampleTimespan.update(tfValidity.getMin());
  mCurrentSampleTimespan.update(tfValidity.getMax());

  mCurrentTimeframeIdRange.update(tfid);

  if (mActivityDuration.isOutside(tfValidity.getMin())) {
    ILOG(Warning, Support) << "Timestamp " << tfValidity.getMin() << " is outside of the assumed run duration ("
                           << mActivityDuration.getMin() << ", " << mActivityDuration.getMax() << ")" << ENDM;
    return;
  }

  if (mWindowLengthMs == 0) {
    mCurrentValidityTimespan = mActivityDuration;
  } else {
    size_t subdivisionIdx = (tfValidity.getMin() - mActivityDuration.getMin()) / mWindowLengthMs;
    size_t fullSubdivisions = mActivityDuration.delta() / mWindowLengthMs;
    if (subdivisionIdx < fullSubdivisions - 1) {
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + subdivisionIdx * mWindowLengthMs);
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + (subdivisionIdx + 1) * mWindowLengthMs);
    } else if (subdivisionIdx == fullSubdivisions - 1) {
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + subdivisionIdx * mWindowLengthMs);
      mCurrentValidityTimespan.update(mActivityDuration.getMax());
    } else { // subdivisionIdx == fullSubdivisions
      mCurrentValidityTimespan.update(mActivityDuration.getMin() + (subdivisionIdx - 1) * mWindowLengthMs);
      mCurrentValidityTimespan.update(mActivityDuration.getMax());
    }
  }
}

void TimekeeperAsynchronous::reset()
{
  mCurrentSampleTimespan = gInvalidValidityInterval;
  mCurrentValidityTimespan = gInvalidValidityInterval;
  mCurrentTimeframeIdRange = gInvalidTimeframeIdRange;
}

template <typename T>
bool not_on_limit(T value)
{
  return value != std::numeric_limits<T>::min() && value != std::numeric_limits<T>::max();
}

validity_time_t
  TimekeeperAsynchronous::activityBoundarySelectionStrategy(validity_time_t ecsTimestamp,
                                                            validity_time_t configTimestamp,
                                                            validity_time_t currentTimestamp,
                                                            std::function<validity_time_t(void)> ccdbTimestampAccessor)
{
  validity_time_t selected = 0;
  auto ccdbTimestamp = ccdbTimestampAccessor == nullptr ? std::numeric_limits<validity_time_t>::min() : ccdbTimestampAccessor();
  if (not_on_limit(ccdbTimestamp)) {
    selected = ccdbTimestamp;
  } else if (not_on_limit(ecsTimestamp)) {
    selected = ecsTimestamp;
  } else if (not_on_limit(configTimestamp)) {
    selected = configTimestamp;
  } else {
    // an exception could be thrown here once the values above are set correctly in production
  }
  ILOG(Debug, Devel) << "Received the following activity boundary propositions: " << ccdbTimestamp << ", " << ecsTimestamp
                     << ", " << configTimestamp << ", " << currentTimestamp << ". Selected: " << selected << ENDM;
  return selected;
}

bool TimekeeperAsynchronous::shouldFinishCycle(const framework::TimingInfo& timingInfo)
{
  // we should start a new window whenever the new data falls outside of the current one.
  // if the window covers the whole run, there is never a reason to finish before we receive an end of stream
  return mCurrentValidityTimespan.isValid() &&
         mWindowLengthMs != 0 &&
         !timingInfo.isTimer() &&
         mCurrentValidityTimespan.isOutside(computeTimestampFromTimeframeID(timingInfo.tfCounter).getMin());
}

ValidityInterval TimekeeperAsynchronous::computeTimestampFromTimeframeID(uint32_t tfid)
{
  if (mOrbitsPerTF == 0) {
    if (auto accessor = getCCDBOrbitsPerTFAccessor()) {
      mOrbitsPerTF = accessor();
      ILOG(Debug, Support) << "Got nOrbitsPerTF " << mOrbitsPerTF << " for TF " << tfid << ENDM;
    } else {
      ILOG(Error, Ops) << "CCDB OrbitsPerTF accessor is not available" << ENDM;
    }
    if (mOrbitsPerTF == 0) {
      ILOG(Error, Ops) << "nHBFperTF from CCDB GRP is 0, object validity will be incorrect" << ENDM;
    }
  }

  auto tfDurationMs = constants::lhc::LHCOrbitNS / 1000000 * mOrbitsPerTF;
  auto tfStart = static_cast<validity_time_t>(mActivityDuration.getMin() + tfDurationMs * (tfid - 1));
  auto tfEnd = static_cast<validity_time_t>(mActivityDuration.getMin() + tfDurationMs * tfid - 1);
  return { tfStart, tfEnd };
}

} // namespace o2::quality_control::core