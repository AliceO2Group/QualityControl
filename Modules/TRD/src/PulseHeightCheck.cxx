// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PulseHeightCheck.cxx
/// \author My Name
///

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TList.h>
#include <TLine.h>
#include <TMath.h>
#include <TPaveText.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include "TRD/PulseHeightCheck.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::trd
{

void PulseHeightCheck::configure()
{
}

Quality PulseHeightCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "pulseheightscaled") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;

      for (int i = 2; i < 6; ++i) {
        if (i > 0 && i < 8 && h->GetBinContent(i) < 50 && h->GetSum() > 100) {
          result = Quality::Bad;
          result.addReason(FlagReasonFactory::Unknown(),
                           "Peak missing " + std::to_string(i));
          break;
        } else if (i > 0 && i < 8 && h->GetBinContent(i) < 100 && h->GetSum() > 100) {
          result = Quality::Medium;
          result.addReason(FlagReasonFactory::Unknown(),
                           "Peak rather low " + std::to_string(i) + " is not empty");
          result.addReason(FlagReasonFactory::BadTracking(),
                           "This is to demonstrate that we can assign more than one Reason to a Quality");
        }
      }
    }
  }
  return result;
}

std::string PulseHeightCheck::getAcceptedType() { return "TH1"; }

void PulseHeightCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "pulseheightscaled") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    //std::string message = fmt::format("Pulseheight message");
    std::string message = "Pulseheight message";
    msg->SetName(message.c_str());
    TLine* lmin = new TLine(1, 0, 1, 1100);
    TLine* lmax = new TLine(7, 0, 7, 1100);

    h->GetListOfFunctions()->Add(lmin);
    h->GetListOfFunctions()->Add(lmax);
    lmin->SetLineColor(kBlue);
    lmin->Draw();
    lmax->SetLineColor(kBlue);
    lmax->Draw();

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
    h->Draw();
  }
}

} // namespace o2::quality_control_modules::trd
