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
/// \file   CheckRawTime.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for TOF Raw data on time distributions
///

// QC
#include "TOF/CheckRawTime.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawTime::configure(std::string)
{
  if (auto param = mCustomParameters.find("MinRawTime"); param != mCustomParameters.end()) {
    mMinRawTime = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawTime"); param != mCustomParameters.end()) {
    mMaxRawTime = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MinPeakRatioIntegral"); param != mCustomParameters.end()) {
    mMinPeakRatioIntegral = ::atof(param->second.c_str());
  }
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckRawTime::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Null;

  if (mo->getName().find("RawsTime") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      mRawTimeMean = h->GetMean();
      const Int_t lowBinId = h->GetXaxis()->FindBin(mMinRawTime);
      const Int_t highBinId = h->GetXaxis()->FindBin(mMaxRawTime);
      mRawTimePeakIntegral = h->Integral(lowBinId, highBinId);
      mRawTimeIntegral = h->Integral(1, h->GetNbinsX());
      if ((mRawTimeMean > mMinRawTime) && (mRawTimeMean < mMaxRawTime)) {
        result = Quality::Good;
      } else {
        if (mRawTimePeakIntegral / mRawTimeIntegral > mMinPeakRatioIntegral) {
          ILOG(Warning, Support) << Form("Raw time: peak/total integral = %5.2f, mean = %5.2f ns -> Check filling scheme...", mRawTimePeakIntegral / mRawTimeIntegral, mRawTimeMean);
          result = Quality::Medium;
        } else {
          ILOG(Warning, Support) << Form("Raw time peak/total integral = %5.2f, mean = %5.2f ns", mRawTimePeakIntegral / mRawTimeIntegral, mRawTimeMean);
          result = Quality::Bad;
        }
      }
    }
  }
  return result;
}

std::string CheckRawTime::getAcceptedType() { return "TH1"; }

void CheckRawTime::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("RawsTime") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (checkResult == Quality::Good) {
      msg->AddText("Mean inside limits: OK");
      msg->AddText(Form("Allowed range: %3.0f-%3.0f ns", mMinRawTime, mMaxRawTime));
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
      msg->AddText(Form("Mean outside limits (%3.0f-%3.0f ns)", mMinRawTime, mMaxRawTime));
      msg->AddText(Form("Raw time peak/total integral = %5.2f%%", mRawTimePeakIntegral * 100. / mRawTimeIntegral));
      msg->AddText(Form("Mean = %5.2f ns", mRawTimeMean));
    } else if (checkResult == Quality::Medium) {
      msg->AddText("No entries. If TOF in the run");
      msg->AddText("email TOF on-call.");
      // text->AddText(Form("Raw time peak/total integral = %5.2f%%", mRawTimePeakIntegral * 100. / mRawTimeIntegral));
      // text->AddText(Form("Mean = %5.2f ns", mRawTimeMean));
      // text->AddText(Form("Allowed range: %3.0f-%3.0f ns", mMinRawTime, mMaxRawTime));
      // text->AddText("If multiple peaks, check filling scheme");
      // text->AddText("See TOF TWiki.");
      // text->SetFillColor(kYellow);
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
  }
}

} // namespace o2::quality_control_modules::tof
