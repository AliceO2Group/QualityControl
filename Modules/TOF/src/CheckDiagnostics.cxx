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
/// \file   CheckDiagnostics.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker dedicated to the study of low level raw data diagnostics words
///

// QC
#include "TOF/CheckDiagnostics.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckDiagnostics::configure(std::string) {}

Quality CheckDiagnostics::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking quality of diagnostic words";

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "DRMCounter") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      }
    }
  }
  return result;
}

std::string CheckDiagnostics::getAcceptedType() { return "TH1F"; }

void CheckDiagnostics::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Info, Support) << "USING BEAUTIFY";
  if (mo->getName() == "DRMCounter") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      ILOG(Info, Support) << "Quality::Good, setting to green";
      msg->Clear();
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("No TOF hits for all events.");
      msg->AddText("Call TOF on-call.");
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. IF TOF IN RUN");
      msg->AddText("check the TOF TWiki");
      msg->SetFillColor(kYellow);
      //
      h->SetFillColor(kOrange);
    } else {
      ILOG(Info, Support) << "Quality::Null, setting to black background";
      msg->SetTextColor(kWhite);
      msg->SetFillColor(kBlack);
    }
  } else
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
}

} // namespace o2::quality_control_modules::tof
