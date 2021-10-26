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
/// \file   CheckRawMultiplicity.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw hit multiplicity obtained with the TaskDigits
///

// QC
#include "TOF/CheckRawMultiplicity.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawMultiplicity::configure(std::string)
{
  if (auto param = mCustomParameters.find("RunningMode"); param != mCustomParameters.end()) {
    mRunningMode = ::atoi(param->second.c_str());
  }
  switch (mRunningMode) {
    case kModeCollisions:
    case kModeCosmics:
      break;
    default:
      ILOG(Fatal, Support) << "Run mode not correct " << mRunningMode << ENDM;
      break;
  }
  if (auto param = mCustomParameters.find("MinRawHits"); param != mCustomParameters.end()) {
    mMinRawHits = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawHits"); param != mCustomParameters.end()) {
    mMaxRawHits = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxFractAtZeroMult"); param != mCustomParameters.end()) {
    mMaxFractAtZeroMult = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxFractAtLowMult"); param != mCustomParameters.end()) {
    mMaxFractAtLowMult = ::atof(param->second.c_str());
  }
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckRawMultiplicity::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "TOFRawsMulti") {
      auto* h = dynamic_cast<TH1I*>(mo->getObject());
      if (h->GetEntries() == 0) { // Histogram is empty
        result = Quality::Medium;
        mShifterMessages.AddMessage("No counts!");
      } else { // Histogram is non empty

        // Computing variables to check
        mRawHitsMean = h->GetMean();
        mRawHitsZeroMultIntegral = h->Integral(1, 1);
        mRawHitsLowMultIntegral = h->Integral(1, 10);
        mRawHitsIntegral = h->Integral(2, h->GetNbinsX());

        if (mRawHitsIntegral == 0) { //if only "0 hits per event" bin is filled -> error
          if (h->GetBinContent(1) > 0) {
            result = Quality::Bad;
            mShifterMessages.AddMessage("Only events at 0 filled!");
          }
        } else {
          const bool isZeroBinContentHigh = (mRawHitsZeroMultIntegral > (mMaxFractAtZeroMult * mRawHitsIntegral));
          const bool isLowMultContentHigh = (mRawHitsLowMultIntegral > (mMaxFractAtLowMult * mRawHitsIntegral));
          const bool isAverageLow = (mRawHitsMean < mMinRawHits);
          const bool isAverageHigh = (mRawHitsMean > mMaxRawHits);
          switch (mRunningMode) {
            case kModeCollisions: // Collisions
              if (isZeroBinContentHigh) {
                result = Quality::Medium;
                mShifterMessages.AddMessage("Zero-multiplicity counts are high");
                mShifterMessages.AddMessage(Form("(%.2f higher than total)!", mMaxFractAtZeroMult));
              } else if (isLowMultContentHigh) {
                result = Quality::Medium;
                mShifterMessages.AddMessage("Low-multiplicity counts are high");
                mShifterMessages.AddMessage(Form("(%.2f higher than total)!", mMaxFractAtLowMult));
              } else if (isAverageLow) {
                result = Quality::Medium;
                mShifterMessages.AddMessage(Form("Average lower than expected (%.2f)!", mMinRawHits));
              } else if (isAverageHigh) {
                result = Quality::Medium;
                mShifterMessages.AddMessage(Form("Average higher than expected (%.2f)!", mMaxRawHits));
              } else {
                result = Quality::Good;
                mShifterMessages.AddMessage("Average within limits");
              }
              break;
            case kModeCosmics: // Cosmics
              if (mRawHitsMean < 10.) {
                result = Quality::Good;
                mShifterMessages.AddMessage("Average within limits");
              } else {
                result = Quality::Medium;
                mShifterMessages.AddMessage("Average outside limits!");
              }
              break;
            default:
              ILOG(Error, Support) << "Not running in correct mode " << mRunningMode;
              break;
          }
        }
      }
    }
  }
  return result;
}

std::string CheckRawMultiplicity::getAcceptedType() { return "TH1I"; }

void CheckRawMultiplicity::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "TOFRawsMulti") {
    auto* h = dynamic_cast<TH1I*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    if (checkResult == Quality::Good) {
      msg->AddText(Form("Mean value = %5.2f", mRawHitsMean));
      msg->AddText(Form("Reference range: %5.2f-%5.2f", mMinRawHits, mMaxRawHits));
      msg->AddText(Form("Events with 0 hits = %5.2f%%", mRawHitsZeroMultIntegral * 100. / mRawHitsIntegral));
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to yellow";
      msg->AddText("IF TOF IN RUN email TOF on-call.");
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
  }
}

} // namespace o2::quality_control_modules::tof
