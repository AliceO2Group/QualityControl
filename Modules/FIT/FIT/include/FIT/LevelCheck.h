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
  void startOfActivity(const Activity& activity) override;

 private:
  void updateBinsToIgnoreWithDCM();
  void setTimestamp(const std::shared_ptr<MonitorObject>& moMetadata);

  std::string mBinsToIgnoreAsStr{ "" };
  std::string mPathDeadChannelMap{ "" };
  std::string mUrlCCDB{ "" };
  std::string mNameObjectToCheck{ "" };
  std::string mMessagePrefixError{ "" };
  std::string mMessagePrefixWarning{ "" };
  std::string mTimestampMetaField{ "timestampMetaField" };
  std::string mTimestampSource{ "" };
  int mNelementsPerLine{ 20 };
  bool mUseBinLabels{ false };
  bool mUseBinError{ false };
  long long mTimestamp{ -1 }; // For fetching CCDB
  std::set<int> mBinsToIgnore{};
  bool mIsInvertedThrsh; // check if values should be upper
  std::string mSignCheck{ "" };
  float mThreshWarning;
  float mThreshError;
  int mNumWarnings;
  int mNumErrors;
  std::vector<double> mVecLabelPos{ 0.15, 0.2, 0.85, 0.45 };
  bool mIsFirstIter{ true };
  ClassDefOverride(LevelCheck, 1);
};

} // namespace o2::quality_control_modules::fit

#endif // QC_MODULE_FIT_LEVELCHECK_H
