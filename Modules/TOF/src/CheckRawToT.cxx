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
/// \file   CheckRawToT.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for TOF Raw data on ToT
///

// QC
#include "TOF/CheckRawToT.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawToT::configure(std::string)
{
  mMinRawToT = 10.; // ns
  mMaxRawToT = 15.; // ns

  if (auto param = mCustomParameters.find("MinRawTime"); param != mCustomParameters.end()) {
    mMinRawToT = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawTime"); param != mCustomParameters.end()) {
    mMaxRawToT = ::atof(param->second.c_str());
  }
}

Quality CheckRawToT::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Null;

  // if ((histname.EndsWith("RawsToT")) || (histname.Contains("RawsToT") && suffixTrgCl)) {
  if (mo->getName().find("RawsToT") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    // if (!suffixTrgCl)
    //   h->SetBit(AliQAv1::GetImageBit(), drawRawsToTSumImage);
    // if (suffixTrgCl) {
    //   h->SetBit(AliQAv1::GetImageBit(), kFALSE);
    //   for (int i = 0; i < nToDrawTrgCl; i++) {
    //     if (histname.EndsWith(ClToDraw[i]))
    //       h->SetBit(AliQAv1::GetImageBit(), kTRUE);
    //   }
    // }
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      const float timeMean = h->GetMean();
      if ((timeMean > mMinRawToT) && (timeMean < mMaxRawToT)) {
        result = Quality::Good;
      } else {
        ILOG(Warning, Support) << Form("ToT mean = %5.2f ns", timeMean);
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string CheckRawToT::getAcceptedType() { return "TH1"; }

void CheckRawToT::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("RawsToT") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "blNDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetBorderSize(1);
    msg->SetTextColor(kWhite);
    msg->SetFillColor(kBlack);
    msg->AddText(Form("Default message for %s", mo->GetName()));
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      ILOG(Info, Support) << "Quality::Good, setting to green";
      msg->Clear();
      msg->AddText("Mean inside limits: OK!!!");
      msg->AddText(Form("Allowed range: %3.1f-%3.1f ns", mMinRawToT, mMaxRawToT));
      msg->SetFillColor(kGreen);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText(Form("Mean outside limits (%3.1f-%3.1f ns)", mMinRawToT, mMaxRawToT));
      msg->AddText("If NOT a technical run,");
      msg->AddText("call TOF on-call.");
      msg->SetFillColor(kRed);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to yellow";
      msg->Clear();
      msg->AddText("No entries. Check TOF TWiki");
      msg->SetFillColor(kYellow);
      msg->SetTextColor(kBlack);
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
  }
}

} // namespace o2::quality_control_modules::tof
