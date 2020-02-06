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
/// \file   TOFCheckDiagnostic.cxx
/// \author Nicolo' Jacazio
///

// QC
#include "TOF/TOFCheckDiagnostic.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

TOFCheckDiagnostic::TOFCheckDiagnostic()
{
}

TOFCheckDiagnostic::~TOFCheckDiagnostic() {}

void TOFCheckDiagnostic::configure(std::string) {}

Quality TOFCheckDiagnostic::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "hDiagnostic") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      }
    }
  }
  return result;
}

std::string TOFCheckDiagnostic::getAcceptedType() { return "TH2F"; }

void TOFCheckDiagnostic::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
#if 0
  if (mo->getName() == "hDiagnostic") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      LOG(INFO) << "Quality::Good, setting to green";
      msg->Clear();
      msg->AddText(Form("Mean value = %5.2f", multiMean));
      msg->AddText(Form("Reference range: %5.2f-%5.2f", minTOFrawhits, maxTOFrawhits));
      msg->AddText(Form("Events with 0 hits = %5.2f%%", zeroBinIntegral * 100. / totIntegral));
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("No TOF hits for all events.");
      msg->AddText("Call TOF on-call.");
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. IF TOF IN RUN");
      msg->AddText("check the TOF TWiki");
      msg->SetFillColor(kYellow);
      //
      h->SetFillColor(kOrange);
    } else {
      LOG(INFO) << "Quality::Null, setting to black background";
      msg->SetFillColor(kBlack);
    }
  } else
    LOG(ERROR) << "Did not get correct histo from " << mo->GetName();
#endif
}

} // namespace o2::quality_control_modules::tof
