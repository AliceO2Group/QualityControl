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
/// \file   TimekeeperAsynchronous.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TIMEKEEPERASYNCHRONOUS_H
#define QUALITYCONTROL_TIMEKEEPERASYNCHRONOUS_H

#include "Timekeeper.h"

namespace o2::quality_control::core
{

class TimekeeperAsynchronous : public Timekeeper
{
 public:
  explicit TimekeeperAsynchronous(validity_time_t windowLengthMs = 0);
  ~TimekeeperAsynchronous() = default;

  void updateByCurrentTimestamp(validity_time_t timestampMs) override;
  void updateByTimeFrameID(uint32_t tfID, uint64_t nOrbitsPerTF) override;
  void reset() override;

 protected:
  validity_time_t activityBoundarySelectionStrategy(validity_time_t ecsTimestamp, validity_time_t configTimestamp,
                                                    validity_time_t currentTimestamp,
                                                    std::function<validity_time_t(void)> ccdbTimestampAccessor) override;

 private:
  validity_time_t mWindowLengthMs = 0;
  bool mWarnedAboutTfIdZero = false;
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_TIMEKEEPERASYNCHRONOUS_H
