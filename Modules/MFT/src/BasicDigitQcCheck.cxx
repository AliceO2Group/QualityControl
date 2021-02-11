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
/// \file   BasicDigitQcCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>
// Quality Control
#include "MFT/BasicDigitQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void BasicDigitQcCheck::configure(std::string) {}

Quality BasicDigitQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName().find("mMFTChipHitMap") != std::string::npos) {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());

      // test it
      if (int(h->GetBinContent(3, 2)) % 3 == 0) {
        result = Quality::Good;
      }
      if (int(h->GetBinContent(3, 2)) % 3 == 1) {
        result = Quality::Medium;
      }
      if (int(h->GetBinContent(3, 2)) % 3 == 2) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string BasicDigitQcCheck::getAcceptedType() { return "TH2"; }

void BasicDigitQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("mMFTChipHitMap") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());

    TPaveText* message = new TPaveText(0.3, 0.8, 0.75, 0.9, "NDC");
    h->GetListOfFunctions()->Add(message);
    message->SetBorderSize(1);

    if (checkResult == Quality::Good) {
      message->AddText("Dummy check status: Good!");
      message->SetFillColor(kGreen + 2);
      message->SetTextColor(kWhite);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      message->AddText("Dummy check status: Bad!");
      message->SetFillColor(kRed + 1);
      message->SetTextColor(kWhite);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::Medium, setting to orange";
      message->AddText("Dummy check status: Medium!");
      message->SetFillColor(kOrange);
      message->SetTextColor(kBlack);
    }
    // h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::mft
