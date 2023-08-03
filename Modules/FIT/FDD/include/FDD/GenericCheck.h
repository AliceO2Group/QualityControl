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
/// LATEST modification for FDD on 25.04.2023 (akhuntia@cern.ch)

#ifndef QC_MODULE_FDD_FDDGENERICCHECK_H
#define QC_MODULE_FDD_FDDGENERICCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include <DataFormatsQualityControl/FlagReasons.h>
#include "DataFormatsFIT/DeadChannelMap.h"
#include <DataFormatsQualityControl/FlagReasons.h>

namespace o2::quality_control_modules::fdd
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
    mBinNumberX = 0;
  };
  bool isActive() { return mIsActive; };

  void doCheck(Quality& result, float checkedValue)
  {
    if (!mIsActive)
      return;

    std::string log = Form("%s : comparing  value = %f with thresholds = %f, %f", mCheckName.c_str(), checkedValue, mThresholdWarning, mThresholdError);
    std::string reason;
    if (mShouldBeLower) {
      if (checkedValue > mThresholdError) {
        if (result.isBetterThan(Quality::Bad))
          result.set(Quality::Bad);
        reason = Form("%.3f > %.3f (%s error limit)", checkedValue, mThresholdError, mCheckName.c_str());
        log += "-> Bad";
      } else if (checkedValue > mThresholdWarning) {
        if (result.isBetterThan(Quality::Medium))
          result.set(Quality::Medium);
        reason = Form("%.3f > %.3f (%s warning limit)", checkedValue, mThresholdWarning, mCheckName.c_str());
        log += "-> Medium";
      } else {
        log += "-> OK";
      }
    } else {
      if (checkedValue < mThresholdError) {
        if (result.isBetterThan(Quality::Bad))
          result.set(Quality::Bad);
        reason = Form("%.3f < %.3f (%s error limit)", checkedValue, mThresholdError, mCheckName.c_str());
        log += "-> Bad";
      } else if (checkedValue < mThresholdWarning) {
        if (result.isBetterThan(Quality::Medium))
          result.set(Quality::Medium);
        reason = Form("%.3f < %.3f (%s warning limit)", checkedValue, mThresholdWarning, mCheckName.c_str());
        log += "-> Medium";
      } else {
        log += "-> OK";
      }
    }
    if (reason.length()) {
      if (mBinNumberX) {
        reason += Form(" for channel %d", mBinNumberX);
      }
      result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), reason);
    }

    ILOG(Debug, Support) << log << ENDM;
  }
  float getThresholdWarning()
  {
    return mThresholdWarning;
  }
  float getThresholdError()
  {
    return mThresholdError;
  }

 public:
  int mBinNumberX;

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
  SingleCheck mCheckMinThresholdY;
  SingleCheck mCheckMaxThresholdY;
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

  constexpr static std::size_t sNCHANNELSPhy = 16;
  o2::fit::DeadChannelMap* mDeadChannelMap;
  std::string mDeadChannelMapStr;
  std::string mPathDeadChannelMap;
};

} // namespace o2::quality_control_modules::fdd

#endif // QC_MODULE_FDD_FDDGENERICCHECK_H
