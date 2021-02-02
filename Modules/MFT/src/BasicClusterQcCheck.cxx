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
/// \file   BasicClusterQcCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TList.h>
// Quality Control
#include "MFT/BasicClusterQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void BasicClusterQcCheck::configure(std::string) {}

Quality BasicClusterQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "mMFT_ClusterSensorID_H") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      // test it
      if ( (int(h->GetBinContent(400)) % 3) == 0) {
        result = Quality::Good;
      }
      if ( (int(h->GetBinContent(400)) % 3) == 1) {
        result = Quality::Medium;
      }
      if ( (int(h->GetBinContent(400)) % 3) == 2) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string BasicClusterQcCheck::getAcceptedType() { return "TH1"; }

void BasicClusterQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "mMFT_ClusterSensorID_H") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    TPaveText *message = new TPaveText(0.3, 0.8, 0.5, 0.9, "NDC");
    h->GetListOfFunctions()->Add(message);
    message->SetBorderSize(1);

    if (checkResult == Quality::Good) {
      h->SetLineColor(kGreen+2);
      message->AddText("Dummy check status: Good!");
      message->SetFillColor(kGreen+2);
      message->SetTextColor(kWhite);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h->SetLineColor(kRed+1);
      message->AddText("Dummy check status: Bad!");
      message->SetFillColor(kRed+1);
      message->SetTextColor(kWhite);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::Medium, setting to orange";
      h->SetLineColor(kOrange);
      message->AddText("Dummy check status: Medium!");
      message->SetFillColor(kOrange);
      message->SetTextColor(kBlack);
    }
    // h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::mft
