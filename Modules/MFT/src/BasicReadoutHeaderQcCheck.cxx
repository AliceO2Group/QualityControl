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
/// \file   BasicReadoutHeaderQcCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TList.h>
// Quality Control
#include "MFT/BasicReadoutHeaderQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void BasicReadoutHeaderQcCheck::configure(std::string) {}

Quality BasicReadoutHeaderQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mMFT_LaneStatus_H") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      // test it: if entries in bins 1 to 4
      int not_ok_status = h->GetBinContent(1) + h->GetBinContent(2) + h->GetBinContent(3) + h->GetBinContent(4);
      if (not_ok_status)
        result = Quality::Bad;
      else
        result = Quality::Good;
    } // end of getting histogram by name
  }   // end of loop over MO
  return result;
}

std::string BasicReadoutHeaderQcCheck::getAcceptedType() { return "TH1"; }

void BasicReadoutHeaderQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "mMFT_LaneStatus_H") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    TPaveText* message = new TPaveText(0.3, 0.8, 0.5, 0.9, "NDC");
    h->GetListOfFunctions()->Add(message);
    message->SetBorderSize(1);

    if (checkResult == Quality::Good) {
      h->SetLineColor(kGreen + 2);
      message->AddText("Check status: Good!");
      message->SetFillColor(kGreen + 2);
      message->SetTextColor(kWhite);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h->SetLineColor(kRed + 1);
      message->AddText("Check status: Bad!");
      message->SetFillColor(kRed + 1);
      message->SetTextColor(kWhite);
    }
  }
}

} // namespace o2::quality_control_modules::mft
