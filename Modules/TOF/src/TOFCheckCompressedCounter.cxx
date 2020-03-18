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
/// \file   TOFCheckCompressedCounter.cxx
/// \author Nicolo' Jacazio
///

// QC
#include "TOF/TOFCheckCompressedCounter.h"
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

void TOFCheckCompressedCounter::configure(std::string) {}

Quality TOFCheckCompressedCounter::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info) << "Checking quality of diagnostic histogram" << ENDM;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "RDHCounterCrate0") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      }
    }
  }
  return result;
}

std::string TOFCheckCompressedCounter::getAcceptedType() { return "TH1F"; }

void TOFCheckCompressedCounter::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Info) << "USING BEAUTIFY";
  if (mo->getName() == "RDHCounterCrate0") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      ILOG(Info) << "Quality::Good, setting to green";
      msg->Clear();
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("No TOF hits for all events.");
      msg->AddText("Call TOF on-call.");
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. IF TOF IN RUN");
      msg->AddText("check the TOF TWiki");
      msg->SetFillColor(kYellow);
      //
      h->SetFillColor(kOrange);
    } else {
      ILOG(Info) << "Quality::Null, setting to black background";
      msg->SetFillColor(kBlack);
    }
  } else
    ILOG(Error) << "Did not get correct histo from " << mo->GetName();
}

} // namespace o2::quality_control_modules::tof
