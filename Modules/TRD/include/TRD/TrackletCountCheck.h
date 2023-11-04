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
  std::string getAcceptedType() override;
  float mThresholdMeanLowPerTrigger, mThresholdMeanHighPerTrigger, mThresholdMeanLowPerTimeFrame, mThresholdMeanHighPerTimeFrame;
  int mStatThresholdPerTrigger;
  Quality mResultPertrigger;
  Quality mResultPerTimeFrame;
  Quality mFinalResult;

  std::shared_ptr<TPaveText> mTrackletPerTriggerMessage;
  std::shared_ptr<TPaveText> mTrackletPerTimeFrameMessage;

  ClassDefOverride(TrackletCountCheck, 2);
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRACKLETCOUNTCHECK_H