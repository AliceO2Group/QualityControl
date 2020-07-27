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

#include "TOF/CheckRawToT.h"

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TList.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

CheckRawToT::CheckRawToT() : minTOFrawTot(10.), // ns
                                     maxTOFrawTot(15.)  // ns
{
}

CheckRawToT::~CheckRawToT() {}

void CheckRawToT::configure(std::string) {}

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
      // flag = AliQAv1::kWARNING;
    } else {
      Float_t timeMean = h->GetMean();
      if ((timeMean > minTOFrawTot) && (timeMean < maxTOFrawTot)) {
        result = Quality::Good;
        // flag = AliQAv1::kINFO;
      } else {
        LOG(WARNING) << Form("ToT mean = %5.2f ns", timeMean);
        result = Quality::Bad;
        // flag = AliQAv1::kERROR;
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
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      LOG(INFO) << "Quality::Good, setting to green";
      //
      msg->Clear();
      msg->AddText("Mean inside limits: OK!!!");
      msg->AddText(Form("Allowed range: %3.1f-%3.1f ns", minTOFrawTot, maxTOFrawTot));
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText(Form("Mean outside limits (%3.1f-%3.1f ns)", minTOFrawTot, maxTOFrawTot));
      msg->AddText("If NOT a technical run,");
      msg->AddText("call TOF on-call.");
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. Check TOF TWiki");
      msg->SetFillColor(kYellow);
      //
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::tof
