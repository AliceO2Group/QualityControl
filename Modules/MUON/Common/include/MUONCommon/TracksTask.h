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
#include "DataFormatsGlobalTracking/RecoContainer.h"
#include <memory>
#include "MUONCommon/TrackPlotter.h"
#include <vector>
#include <array>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::muon
{

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;

class TracksTask /*final*/ : public TaskInterface
{
 public:
  enum matchType : int8_t { MCH = 0,
                            MCHMID,
                            MFTMCH,
                            MFTMCHMID,
                            SIZE };

  TracksTask();
  ~TracksTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  /** check whether all the expected inputs are present.*/
  bool assertInputs(o2::framework::ProcessingContext& ctx);
  bool getBooleanParam(const char* paramName) const;

  template <class T>
  T getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity);
  template <class T>
  T getParameter(std::string parName, const T defaultValue);

 private:
  std::map<GID::Source, std::unique_ptr<TrackPlotter>> mTrackPlotters;
  std::map<GID::Source, std::unique_ptr<TrackPlotter>> mTrackPlottersWithCuts;
  std::shared_ptr<o2::globaltracking::DataRequest> mDataRequest;
  o2::globaltracking::RecoContainer mRecoCont;
  GID::mask_t mSrc = GID::getSourcesMask("MCH-MID");
  GID::mask_t mAllowedSources = GID::getSourcesMask("MFT,MCH,MID,MCH-MID,MFT-MCH,MFT-MCH-MID");

  // MCH-MID
  gsl::span<const o2::dataformats::TrackMCHMID> mMCHMIDTracks;
  // MFT-MCH
  gsl::span<const o2::dataformats::GlobalFwdTrack> mMFTMCHTracks;
  // MFT-MCH-MID
  gsl::span<const o2::dataformats::GlobalFwdTrack> mMFTMCHMIDTracks;
};

template <class T>
T TracksTask::getParameter(std::string parName, const T defaultValue, const o2::quality_control::core::Activity& activity)
{
  T result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    std::stringstream ss(parOpt.value());
    ss >> result;
  }
  return result;
}

template <class T>
T TracksTask::getParameter(std::string parName, const T defaultValue)
{
  T result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    std::stringstream ss(parOpt.value());
    ss >> result;
  }
  return result;
}

} // namespace o2::quality_control_modules::muon

#endif
