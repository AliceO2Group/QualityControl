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
/// \file   TrackletCountCheck.h
/// \author Deependra Sharma @IITB
///

#ifndef QC_MODULE_TRD_TRDTRACKLETCOUNTCHECK_H
#define QC_MODULE_TRD_TRDTRACKLETCOUNTCHECK_H

#include <TPaveText.h>
#include <TH1F.h>
#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::trd
{
class TrackletCountCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  TrackletCountCheck() = default;
  /// Destructor
  ~TrackletCountCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  void reset() override;
  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;

 private:
  float mThresholdMeanLowPerTrigger, mThresholdMeanHighPerTrigger, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame;
  int mStatThresholdPerTrigger;

  float mTrackletPerTimeFrameThreshold, mRatioThreshold, mZeroBinRatioThreshold;

  Quality mResultPertrigger;
  Quality mResultPerTimeFrame;
  Quality mFinalResult;

  std::shared_ptr<TPaveText> mTrackletPerTriggerMessage;
  std::shared_ptr<TPaveText> mTrackletPerTimeFrameMessage;

  /// @brief This function do comparsion between number of TimeFrame with lower and higher tracklets
  /// @param trackletPerTimeFrameThreshold The deciding boundary between Timeframes of lower and higher tracklets
  /// @param acceptedRatio accepted ratio (Timeframe with lower tracklets/ Timeframe with higher tracklets)
  /// @param zeroTrackletTfRation  accepted ratio (Timeframe without tracklets/ Timeframe with Tracklets)
  /// @param hist TrackletPerTimeFrame distribution
  /// @return true if ratio (Timeframe with lower tracklets/ Timeframe with higher tracklets) is below than accepted ratio
  bool isTrackletDistributionAccepeted(float trackletPerTimeFrameThreshold, float acceptedRatio, float zeroTrackletTfRatio, TH1F* hist);

  /// @brief This function checks if ration of TF without tracklets to TF with tracklets is higher than a threshold
  /// @param zeroTrackletTfRation accepted ratio (Timeframe without tracklets/ Timeframe with Tracklets)
  /// @param hist TrackletPerTimeFrame distribution
  /// @return true if ratio (TF WO tracklets / TF W tracklets) is lower than a threshold
  bool isTimeframeRatioWOTrackletAccepted(float zeroTrackletTfRatio, TH1F* hist);

  std::shared_ptr<Activity> mActivity;
  ClassDefOverride(TrackletCountCheck, 2);
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRACKLETCOUNTCHECK_H