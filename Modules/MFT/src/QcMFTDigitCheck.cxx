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
/// \file   QcMFTDigitCheck.cxx
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
#include "MFT/QcMFTDigitCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTDigitCheck::configure(std::string) {}

Quality QcMFTDigitCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName().find("mMFTChipOccupancyStdDev") != std::string::npos) {
      auto* histogram = dynamic_cast<TH2F*>(mo->getObject());

      // test it
      if (histogram->GetBinContent(histogram->GetMinimumBin()) > 250) {
        result = Quality::Good;
      }
      if ((histogram->GetBinContent(histogram->GetMinimumBin()) < 250) && (histogram->GetBinContent(histogram->GetMinimumBin()) > 200)) {
        result = Quality::Medium;
      }
      if (histogram->GetBinContent(histogram->GetMinimumBin()) < 200) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string QcMFTDigitCheck::getAcceptedType() { return "TH2"; }

void QcMFTDigitCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("mMFTChipOccupancyStdDev") != std::string::npos) {
    auto* histogram = dynamic_cast<TH2F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      LOG(INFO) << "Quality::Good";
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[418]{Dummy check status: Good!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad";
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[633]{Dummy check status: Bad!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::Medium";
      TLatex* tl = new TLatex(350, 1.05 * histogram->GetMaximum(), "#color[800]{Dummy check status: Medium!}");
      histogram->GetListOfFunctions()->Add(tl);
      tl->Draw();
    }
  }
}

} // namespace o2::quality_control_modules::mft
