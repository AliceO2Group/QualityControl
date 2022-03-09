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
/// \file   QcMFTClusterCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include <TList.h>
// Quality Control
#include "MFT/QcMFTClusterCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTClusterCheck::configure() {}

Quality QcMFTClusterCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "mClusterOccupancy") {
      auto* histogram = dynamic_cast<TH1F*>(mo->getObject());

      // test it
      if ((int(histogram->GetBinContent(400)) % 3) == 0) {
        // result = Quality::Good;
      }
      if ((int(histogram->GetBinContent(400)) % 3) == 1) {
        // result = Quality::Medium;
      }
      if ((int(histogram->GetBinContent(400)) % 3) == 2) {
        // result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string QcMFTClusterCheck::getAcceptedType() { return "TH1"; }

void QcMFTClusterCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "mClusterOccupancy") {
    auto* histogram = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      LOG(info) << "Quality::Good";
      histogram->SetLineColor(kGreen + 2);
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[418]{Dummy check status: Good!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad";
      histogram->SetLineColor(kRed + 1);
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[633]{Dummy check status: Bad!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::Medium";
      histogram->SetLineColor(kOrange);
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[800]{Dummy check status: Medium!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    }
  }
}

} // namespace o2::quality_control_modules::mft
