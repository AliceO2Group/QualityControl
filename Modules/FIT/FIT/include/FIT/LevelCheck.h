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
/// \file   RatioCheck.h
/// \author Artur Furs afurs@cern.ch
///

#ifndef QC_MODULE_FIT_LEVELCHECK_H
#define QC_MODULE_FIT_LEVELCHECK_H

#include <regex>
#include <numeric>
#include <set>

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::fit
{

class LevelCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  LevelCheck() = default;
  ~LevelCheck() override = default;

  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  void updateBinsToIgnoreWithDCM();

  std::string mBinsToIgnoreAsStr{ "" };
  std::string mPathDeadChannelMap{ "" };
  std::string mUrlCCDB{ "" };
  std::string mNameObjectToCheck{ "" };
  std::set<int> mBinsToIgnore{};
  bool mIsInvertedThrsh; // check if values should be upper
  std::string mSignCheck{ "" };
  float mThreshWarning;
  float mThreshError;
  int mNumWarnings;
  int mNumErrors;
  std::vector<double> mVecLabelPos{ 0.15, 0.2, 0.85, 0.45 };
  ClassDefOverride(LevelCheck, 1);
};

} // namespace o2::quality_control_modules::fit

#endif // QC_MODULE_FIT_LEVELCHECK_H
