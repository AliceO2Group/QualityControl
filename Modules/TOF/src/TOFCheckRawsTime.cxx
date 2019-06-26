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
/// \file   TOFCheckRawsTime.cxx
/// \author Nicolo' Jacazio
///

#include "TOF/TOFCheckRawsTime.h"

// ROOT
#include <FairLogger.h>
#include <TH1.h>
#include <TList.h>
#include <TMath.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

TOFCheckRawsTime::TOFCheckRawsTime() : minTOFrawTime(175.f),
                                       maxTOFrawTime(250.f),
                                       timeMean(0),
                                       peakIntegral(0),
                                       totIntegral(0)
{
}

TOFCheckRawsTime::~TOFCheckRawsTime() {}

void TOFCheckRawsTime::configure(std::string)
{
  // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
  //   minTOFrawTime = 150.; //ns
  //   maxTOFrawTime = 250.; //ns
  // }
}

Quality TOFCheckRawsTime::check(const MonitorObject* mo)
{
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
      timeMean = h->GetMean();
      Int_t lowBinId = h->GetXaxis()->FindBin(minTOFrawTime);
      Int_t highBinId = h->GetXaxis()->FindBin(maxTOFrawTime);
      peakIntegral = h->Integral(lowBinId, highBinId);
      totIntegral = h->Integral(1, h->GetNbinsX());
      if ((timeMean > minTOFrawTime) && (timeMean < maxTOFrawTime)) {

        result = Quality::Good;
        // flag = AliQAv1::kINFO;
      } else {
        if (peakIntegral / totIntegral > 0.20) {
          LOG(WARNING) << Form("Raw time: peak/total integral = %5.2f, mean = %5.2f ns -> Check filling scheme...", peakIntegral / totIntegral, timeMean);
          result = Quality::Medium;
          // flag = AliQAv1::kWARNING;
        } else {
          LOG(WARNING) << Form("Raw time peak/total integral = %5.2f, mean = %5.2f ns", peakIntegral / totIntegral, timeMean);
          result = Quality::Bad;
          // flag = AliQAv1::kERROR;
        }
      }
    }
  }
  return result;
}

std::string TOFCheckRawsTime::getAcceptedType() { return "TH1"; }

void TOFCheckRawsTime::beautify(MonitorObject* mo, Quality checkResult)
{
  if (mo->getName().find("RawsTime") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Mean inside limits: OK!!!");
      msg->AddText(Form("Allowed range: %3.0f-%3.0f ns", minTOFrawTime, maxTOFrawTime));
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call TOF on-call.");
      msg->AddText(Form("Mean outside limits (%3.0f-%3.0f ns)", minTOFrawTime, maxTOFrawTime));
      msg->AddText(Form("Raw time peak/total integral = %5.2f%%", peakIntegral * 100. / totIntegral));
      msg->AddText(Form("Mean = %5.2f ns", timeMean));
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. If TOF in the run");
      msg->AddText("check TOF TWiki");
      msg->SetFillColor(kYellow);
      // text->Clear();
      // text->AddText(Form("Raw time peak/total integral = %5.2f%%", peakIntegral * 100. / totIntegral));
      // text->AddText(Form("Mean = %5.2f ns", timeMean));
      // text->AddText(Form("Allowed range: %3.0f-%3.0f ns", minTOFrawTime, maxTOFrawTime));
      // text->AddText("If multiple peaks, check filling scheme");
      // text->AddText("See TOF TWiki.");
      // text->SetFillColor(kYellow);
      //
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::tof
