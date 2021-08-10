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
/// \file   QcMFTReadoutCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
// Quality Control
#include "MFT/QcMFTReadoutCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTReadoutCheck::configure(std::string) {}

Quality QcMFTReadoutCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mMFTSummaryLaneStatus") {
      auto* histogram = dynamic_cast<TH1F*>(mo->getObject());

      // test it: if entries in bins 1 to 4
      int notOkStatus = histogram->GetBinContent(1) + histogram->GetBinContent(2) + histogram->GetBinContent(3) + histogram->GetBinContent(4);
      if (notOkStatus)
        result = Quality::Bad;
      else
        result = Quality::Good;
    } // end of getting histogram by name
  }   // end of loop over MO
  return result;
}

std::string QcMFTReadoutCheck::getAcceptedType() { return "TH1, TH2"; }

void QcMFTReadoutCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "mMFTSummaryLaneStatus") {
    auto* histogram = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      LOG(INFO) << "Quality::Good, setting to green";
      histogram->SetLineColor(kGreen + 2);
      TLatex* tl = new TLatex(1.4, 1.05 * histogram->GetMaximum(), "#color[418]{Check status: Good!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      histogram->SetLineColor(kRed + 1);
      TLatex* tl = new TLatex(1.4, 1.05 * histogram->GetMaximum(), "#color[633]{Check status: Bad!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    }
  }
}

} // namespace o2::quality_control_modules::mft
