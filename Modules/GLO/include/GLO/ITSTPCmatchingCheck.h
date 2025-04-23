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

/// \file   ITSTPCmatchingCheck.h
/// \brief  Check for ITS-TPC sync. matching efficiency
/// \author felix.schlepper@cern.ch

#ifndef QC_MODULE_GLO_GLOITSTPCMATCHINGCHECK_H
#define QC_MODULE_GLO_GLOITSTPCMATCHINGCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/Quality.h"

#include "GLO/Helpers.h"

#include <vector>

namespace o2::quality_control_modules::glo
{

/// \brief  Check for ITS-TPC sync. matching efficiency
/// \author felix.schlepper@cern.ch
class ITSTPCmatchingCheck final : public o2::quality_control::checker::CheckInterface
{
 public:
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) final;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) final;
  void startOfActivity(const Activity& activity) final;

 private:
  static std::vector<std::pair<int, int>> findRanges(const std::vector<int>& nums) noexcept;

  std::shared_ptr<Activity> mActivity;

  // Pt
  bool mShowPt{ false };
  float mMinPt{ 1. };
  float mMaxPt{ 1.999 };
  float mThresholdPt{ 0.5 };

  // Phi
  bool mShowPhi{ false };
  float mThresholdPhi{ 0.3 };

  // Eta
  bool mShowEta{ false };
  float mThresholdEta{ 0.4 };
  float mMinEta{ -0.8 };
  float mMaxEta{ 0.8 };

  // K0s
  bool mShowK0s{ false };
  float mAccRelError{ 0.02 };
  float mAccUncertainty{ 2 };
  helpers::K0sFitter mK0sFitter;

  // Other
  int mLimitRange{ -1 };
  bool mIsPbPb{ false };

  ClassDefOverride(ITSTPCmatchingCheck, 2);
};

} // namespace o2::quality_control_modules::glo

#endif // QC_MODULE_GLO_GLOITSTPCMATCHINGCHECK_H
