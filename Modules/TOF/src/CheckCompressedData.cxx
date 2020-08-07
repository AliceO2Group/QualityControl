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
/// \file   CheckCompressedData.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw compressed data for TOF
///

// QC
#include "TOF/CheckCompressedData.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckCompressedData::configure(std::string)
{
  mDiagnosticThresholdPerSlot = 0;
  if (auto param = mCustomParameters.find("DiagnosticThresholdPerSlot"); param != mCustomParameters.end()) {
    mDiagnosticThresholdPerSlot = ::atof(param->second.c_str());
  }
}

Quality CheckCompressedData::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info) << "Checking quality of compressed data";

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "hDiagnostic") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      result = Quality::Good;
      for (int i = 1; i < h->GetNbinsX(); i++) {
        for (int j = 1; j < h->GetNbinsY(); j++) {
          const float content = h->GetBinContent(i, j);
          if (content > mDiagnosticThresholdPerSlot) { // If above threshold
            result = Quality::Bad;
          } else if (content > 0) { // If larger than zero
            result = Quality::Medium;
          }
        }
      }
    }
  }
  return result;
}

std::string CheckCompressedData::getAcceptedType() { return "TH2F"; }

void CheckCompressedData::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "hDiagnostic") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.9, 0.1, 1.0, 0.5, "blNDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetBorderSize(1);
    msg->SetTextColor(kWhite);
    msg->SetFillColor(kBlack);
    msg->AddText("Default message for hDiagnostic");
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      ILOG(Info) << "Quality::Good, setting to green";
      msg->Clear();
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Diagnostics");
      msg->AddText("above");
      msg->AddText(Form("threshold (%.0f)", mDiagnosticThresholdPerSlot));
      msg->SetFillColor(kRed);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to yellow";
      msg->Clear();
      msg->AddText("Diagnostics above zero");
      msg->SetFillColor(kYellow);
      msg->SetTextColor(kBlack);
    }
  } else {
    ILOG(Error) << "Did not get correct histo from " << mo->GetName();
  }
}
} // namespace o2::quality_control_modules::tof
