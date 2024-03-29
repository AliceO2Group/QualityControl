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
/// \file   CheckRaw.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw compressed data for TOF
///

// QC
#include "TOF/CheckRaw.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRaw::configure(std::string)
{
  mDiagnosticThresholdPerSlot = 0;
  if (auto param = mCustomParameters.find("DiagnosticThresholdPerSlot"); param != mCustomParameters.end()) {
    mDiagnosticThresholdPerSlot = ::atof(param->second.c_str());
  }
}

Quality CheckRaw::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking quality of raw data" << ENDM;

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
    } else if (mo->getName() == "DRMCounter") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      }
    } else if (mo->getName() == "LTMCounter") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      }
    }
  }
  return result;
}

std::string CheckRaw::getAcceptedType() { return "TH2F"; }

void CheckRaw::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
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
      ILOG(Info, Support) << "Quality::Good, setting to green" << ENDM;
      msg->Clear();
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red" << ENDM;
      msg->Clear();
      msg->AddText("Diagnostics");
      msg->AddText("above");
      msg->AddText(Form("threshold (%.0f)", mDiagnosticThresholdPerSlot));
      msg->SetFillColor(kRed);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to yellow" << ENDM;
      msg->Clear();
      msg->AddText("Diagnostics above zero");
      msg->SetFillColor(kYellow);
      msg->SetTextColor(kBlack);
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
  }
}
} // namespace o2::quality_control_modules::tof
