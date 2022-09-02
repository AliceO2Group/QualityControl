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
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Checker for the meassured time obtained with the TaskDigits
///

// QC
#include "TOF/CheckRawTime.h"
#include "TOF/Utils.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::tof
{

void CheckRawTime::configure()
{
  utils::parseDoubleParameter(mCustomParameters, "MinEntriesBeforeMessage", mMinEntriesBeforeMessage);
  utils::parseFloatParameter(mCustomParameters, "MinAllowedTime", mMinAllowedTime);
  utils::parseFloatParameter(mCustomParameters, "MaxAllowedTime", mMaxAllowedTime);
  utils::parseFloatParameter(mCustomParameters, "MinPeakRatioIntegral", mMinPeakRatioIntegral);
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckRawTime::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    if (!isObjectCheckable(mo)) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which is not of type " << getAcceptedType() << ENDM;
      continue;
    }
    ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;

    if (mo->getName().find("Time/") != std::string::npos) {
      auto* h = static_cast<TH1F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
        result.addReason(FlagReasonFactory::NoDetectorData(),
                         "Empty histogram (no counts)");
      } else {
        mRawTimeMean = h->GetMean();
        static const int lowBinId = h->GetXaxis()->FindBin(mMinAllowedTime);
        static const int highBinId = h->GetXaxis()->FindBin(mMaxAllowedTime);
        mRawTimePeakIntegral = h->Integral(lowBinId, highBinId);
        mRawTimeIntegral = h->Integral(1, h->GetNbinsX());
        if ((mRawTimeMean > mMinAllowedTime) && (mRawTimeMean < mMaxAllowedTime)) {
          result = Quality::Good;
        } else {
          if (mRawTimePeakIntegral / mRawTimeIntegral > mMinPeakRatioIntegral) {
            ILOG(Warning, Support) << Form("Raw time: peak/total integral = %5.2f, mean = %5.2f ns -> Check filling scheme...", mRawTimePeakIntegral / mRawTimeIntegral, mRawTimeMean) << ENDM;
            result = Quality::Medium;
            result.addReason(FlagReasonFactory::Unknown(),
                             "Peak over total outside of allowed range");
          } else {
            ILOG(Warning, Support) << Form("Raw time peak/total integral = %5.2f, mean = %5.2f ns", mRawTimePeakIntegral / mRawTimeIntegral, mRawTimeMean) << ENDM;
            result = Quality::Bad;
            result.addReason(FlagReasonFactory::Unknown(),
                             "Time mean out of expected range");
          }
        }
      }
    }
  }
  return result;
}

void CheckRawTime::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Debug, Devel) << "Beautifying " << mo->getName() << ENDM;
  if (!isObjectCheckable(mo)) {
    ILOG(Error, Support) << "Cannot beautify MO " << mo->getName() << " which is not of type " << getAcceptedType() << ENDM;
    return;
  }
  if (mo->getName().find("Time/") != std::string::npos) {
    auto* h = static_cast<TH1F*>(mo->getObject());
    if (h->GetEntries() < mMinEntriesBeforeMessage) { // Checking that the histogram has enough entries before printing messages
      return;
    }

    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    if (checkResult == Quality::Good) {
      msg->AddText("Mean inside limits: OK");
      msg->AddText(Form("Allowed range: %3.0f-%3.0f ns", mMinAllowedTime, mMaxAllowedTime));
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
      msg->AddText(Form("Mean outside limits (%3.0f-%3.0f ns)", mMinAllowedTime, mMaxAllowedTime));
      msg->AddText(Form("Raw time peak/total integral = %5.2f%%", mRawTimePeakIntegral * 100. / mRawTimeIntegral));
      msg->AddText(Form("Mean = %5.2f ns", mRawTimeMean));
    } else if (checkResult == Quality::Medium) {
      msg->AddText("No entries. If TOF in the run");
      msg->AddText("email TOF on-call.");
      // text->AddText(Form("Raw time peak/total integral = %5.2f%%", mRawTimePeakIntegral * 100. / mRawTimeIntegral));
      // text->AddText(Form("Mean = %5.2f ns", mRawTimeMean));
      // text->AddText(Form("Allowed range: %3.0f-%3.0f ns", mMinAllowedTime, mMaxAllowedTime));
      // text->AddText("If multiple peaks, check filling scheme");
      // text->AddText("See TOF TWiki.");
      // text->SetFillColor(kYellow);
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
