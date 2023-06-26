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
/// \file   Timekeeper.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TIMEKEEPER_H
#define QUALITYCONTROL_TIMEKEEPER_H

#include "QualityControl/ValidityInterval.h"
#include <functional>

namespace o2::framework
{
struct ProcessingContext;
struct TimingInfo;
} // namespace o2::framework

namespace o2::quality_control::core
{

// could be moved to a separate file
using TimeframeIdRange = o2::math_utils::detail::Bracket<uint32_t>;
const static TimeframeIdRange gInvalidTimeframeIdRange{
  std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::min()
};

class Timekeeper
{
 public:
  Timekeeper();
  virtual ~Timekeeper() = default;

  /// \brief sets activity (run) duration
  void setActivityDuration(ValidityInterval);
  /// \brief sets start of activity (run), but prioritises the source of information according to the class implementation
  void setStartOfActivity(validity_time_t ecsTimestamp = 0, validity_time_t configTimestamp = 0,
                          validity_time_t currentTimestamp = 0, std::function<validity_time_t(void)> ccdbTimestampAccessor = nullptr);
  /// \brief sets end of activity (run), but prioritises the source of information according to the class implementation
  void setEndOfActivity(validity_time_t ecsTimestamp = 0, validity_time_t configTimestamp = 0, validity_time_t currentTimestamp = 0,
                        std::function<validity_time_t(void)> ccdbTimestampAccessor = nullptr);

  /// \brief updates the validity based on the provided timestamp (ms since epoch)
  virtual void updateByCurrentTimestamp(validity_time_t timestampMs) = 0;
  /// \brief updates the validity based on the provided TF ID
  virtual void updateByTimeFrameID(uint32_t tfID, uint64_t nOrbitsPerTF) = 0;

  /// \brief resets the state of the mCurrent* counters
  virtual void reset() = 0;

  ValidityInterval getValidity() const;
  ValidityInterval getSampleTimespan() const;
  TimeframeIdRange getTimerangeIdRange() const;
  ValidityInterval getActivityDuration() const;

 protected:
  /// \brief defines how a class implementation chooses the activity (run) boundaries
  // by using an accessor to ccdb, we do not call if we are not interested
  virtual validity_time_t
    activityBoundarySelectionStrategy(validity_time_t ecsTimestamp,
                                      validity_time_t configTimestamp,
                                      validity_time_t currentTimestamp,
                                      std::function<validity_time_t(void)> ccdbTimestampAccessor) = 0;

 protected:
  ValidityInterval mActivityDuration = gInvalidValidityInterval;        // from O2StartTime to O2EndTime or current timestamp
  ValidityInterval mCurrentValidityTimespan = gInvalidValidityInterval; // since the last reset time until `update()` call
  ValidityInterval mCurrentSampleTimespan = gInvalidValidityInterval;   // since the last reset
  TimeframeIdRange mCurrentTimeframeIdRange = gInvalidTimeframeIdRange; // since the last reset
};

} // namespace o2::quality_control::core
#endif // QUALITYCONTROL_TIMEKEEPER_H
