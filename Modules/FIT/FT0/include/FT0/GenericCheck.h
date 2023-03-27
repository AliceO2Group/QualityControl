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
/// \file   GenericCheck.h
/// \author Sebastian Bysiak
///

#ifndef QC_MODULE_FT0_FT0GENERICCHECK_H
#define QC_MODULE_FT0_FT0GENERICCHECK_H

#include "QualityControl/CheckInterface.h"
#include <DataFormatsQualityControl/FlagReasons.h>
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control_modules::ft0
{

/// \brief  helper class to store acceptable limits for given quantity
/// \author Sebastian Bysiak
class SingleCheck
{
 public:
  SingleCheck() = default;
  ~SingleCheck() = default;

  SingleCheck(std::string name, float thresholdWarning, float thresholdError, bool shouldBeLower, bool isActive)
  {
    mCheckName = name;
    mThresholdWarning = thresholdWarning;
    mThresholdError = thresholdError;
    mShouldBeLower = shouldBeLower;
    mIsActive = isActive;
  };
  bool isActive() { return mIsActive; };

  void doCheck(Quality& result, float checkedValue)
  {
    if (!mIsActive)
      return;

    std::string log = Form("%s : comparing  value = %f with thresholds = %f, %f", mCheckName.c_str(), checkedValue, mThresholdWarning, mThresholdError);
    if (mShouldBeLower) {
      if (checkedValue > mThresholdError) {
        if (result.isBetterThan(Quality::Bad))
          result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("%.3f > %.3f (%s error limit)", checkedValue, mThresholdError, mCheckName.c_str()));
        log += "-> Bad";
      } else if (checkedValue > mThresholdWarning) {
        if (result.isBetterThan(Quality::Medium))
          result.set(Quality::Medium);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("%.3f > %.3f (%s warning limit)", checkedValue, mThresholdWarning, mCheckName.c_str()));
        log += "-> Medium";
      } else {
        log += "-> OK";
      }
    } else {
      if (checkedValue < mThresholdError) {
        if (result.isBetterThan(Quality::Bad))
          result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("%.3f < %.3f (%s error limit)", checkedValue, mThresholdError, mCheckName.c_str()));
        log += "-> Bad";
      } else if (checkedValue < mThresholdWarning) {
        if (result.isBetterThan(Quality::Medium))
          result.set(Quality::Medium);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("%.3f < %.3f (%s warning limit)", checkedValue, mThresholdWarning, mCheckName.c_str()));
        log += "-> Medium";
      } else {
        log += "-> OK";
      }
    }
    ILOG(Debug, Support) << log << ENDM;
  }

 private:
  std::string mCheckName;
  float mThresholdWarning;
  float mThresholdError;
  bool mShouldBeLower;
  bool mIsActive;
};

/// \brief  checks multiple basic hist statistics
/// \author Sebastian Bysiak
class GenericCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  GenericCheck() = default;
  /// Destructor
  ~GenericCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(GenericCheck, 2);

 private:
  SingleCheck getCheckFromConfig(std::string);

  SingleCheck mCheckMaxOverflowIntegralRatio;

  SingleCheck mCheckMinMeanX;
  SingleCheck mCheckMaxMeanX;
  SingleCheck mCheckMaxStddevX;

  SingleCheck mCheckMinMeanY;
  SingleCheck mCheckMaxMeanY;
  SingleCheck mCheckMaxStddevY;

  SingleCheck mCheckMinGraphLastPoint;
  SingleCheck mCheckMaxGraphLastPoint;

  std::array<double, 4> mPositionMsgBox;
  std::string mNameObjOnCanvas;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_FT0GENERICCHECK_H
