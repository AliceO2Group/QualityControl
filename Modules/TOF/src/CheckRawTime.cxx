// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CheckRawTime.cxx
/// \author Nicolo' Jacazio
///

#include "TOF/CheckRawTime.h"

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TList.h>
#include <TMath.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawTime::configure(std::string)
{
  mMinRawTime = 175.f;
  mMaxRawTime = 250.f;
  if (auto param = mCustomParameters.find("MinRawTime"); param != mCustomParameters.end()) {
    mMinRawTime = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawTime"); param != mCustomParameters.end()) {
    mMaxRawTime = ::atof(param->second.c_str());
  }
  // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
  //   minTOFrawTime = 150.; //ns
  //   maxTOFrawTime = 250.; //ns
  // }
}

Quality CheckRawTime::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Null;

  // const Double_t binWidthTOFrawTime = 2.44;

  // if ((histname.EndsWith("RawsTime")) || (histname.Contains("RawsTime") && suffixTrgCl)) {
  if (mo->getName().find("RawsTime") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    // if (!suffixTrgCl)
    //   h->SetBit(AliQAv1::GetImageBit(), drawRawsTimeSumImage);
    // if (suffixTrgCl) {
    //   h->SetBit(AliQAv1::GetImageBit(), kFALSE);
    //   for (int i = 0; i < nToDrawTrgCl; i++) {
    //     if (histname.EndsWith(ClToDraw[i]))
    //       h->SetBit(AliQAv1::GetImageBit(), kTRUE);
    //   }
    // }
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
      // flag = AliQAv1::kWARNING;
    } else {
      mRawTimeMean = h->GetMean();
      const Int_t lowBinId = h->GetXaxis()->FindBin(mMinRawTime);
      const Int_t highBinId = h->GetXaxis()->FindBin(mMaxRawTime);
      mRawTimePeakIntegral = h->Integral(lowBinId, highBinId);
      mRawTimeIntegral = h->Integral(1, h->GetNbinsX());
      if ((mRawTimeMean > mMinRawTime) && (mRawTimeMean < mMaxRawTime)) {
        result = Quality::Good;
      } else {
        if (mRawTimePeakIntegral / mRawTimeIntegral > 0.20) {
          LOG(WARNING) << Form("Raw time: peak/total integral = %5.2f, mean = %5.2f ns -> Check filling scheme...", mRawTimePeakIntegral / mRawTimeIntegral, mRawTimeMean);
          result = Quality::Medium;
        } else {
          LOG(WARNING) << Form("Raw time peak/total integral = %5.2f, mean = %5.2f ns", mRawTimePeakIntegral / mRawTimeIntegral, mRawTimeMean);
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
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "blNDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetBorderSize(1);
    msg->SetTextColor(kWhite);
    msg->SetFillColor(kBlack);
    msg->AddText("Default message for RawsTime");
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      ILOG(Info) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Mean inside limits: OK!!!");
      msg->AddText(Form("Allowed range: %3.0f-%3.0f ns", mMinRawTime, mMaxRawTime));
      msg->SetFillColor(kGreen);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Call TOF on-call.");
      msg->AddText(Form("Mean outside limits (%3.0f-%3.0f ns)", mMinRawTime, mMaxRawTime));
      msg->AddText(Form("Raw time peak/total integral = %5.2f%%", mRawTimePeakIntegral * 100. / mRawTimeIntegral));
      msg->AddText(Form("Mean = %5.2f ns", mRawTimeMean));
      msg->SetFillColor(kRed);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to yellow";
      //
      msg->Clear();
      msg->AddText("No entries. If TOF in the run");
      msg->AddText("check TOF TWiki");
      msg->SetFillColor(kYellow);
      msg->SetTextColor(kBlack);
      // text->Clear();
      // text->AddText(Form("Raw time peak/total integral = %5.2f%%", mRawTimePeakIntegral * 100. / mRawTimeIntegral));
      // text->AddText(Form("Mean = %5.2f ns", mRawTimeMean));
      // text->AddText(Form("Allowed range: %3.0f-%3.0f ns", mMinRawTime, mMaxRawTime));
      // text->AddText("If multiple peaks, check filling scheme");
      // text->AddText("See TOF TWiki.");
      // text->SetFillColor(kYellow);
    }

  } else {
    ILOG(Error) << "Did not get correct histo from " << mo->GetName();
  }
}

} // namespace o2::quality_control_modules::tof
