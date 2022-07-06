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

#ifndef QC_MODULE_MUON_COMMON_TRACKS_TASK_H
#define QC_MODULE_MUON_COMMON_TRACKS_TASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include "MUONCommon/TrackPlotter.h"
#include <vector>
#include <array>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::muon
{

class TracksTask /*final*/ : public TaskInterface
{
 public:
  TracksTask();
  ~TracksTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  /** check whether all the expected inputs are present.*/
  bool assertInputs(o2::framework::ProcessingContext& ctx);
  void publish(const TrackPlotter& tp);
  bool getBooleanParam(const char* paramName) const;

 private:
  std::unique_ptr<muon::TrackPlotter> mTrackPlotter;
  bool mUseMatchedMCHMIDTracks{ false };
};

} // namespace o2::quality_control_modules::muon

#endif
