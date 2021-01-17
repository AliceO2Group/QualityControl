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

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
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
    if (mo->getName() == "ChipHitMaps/mMFT_chip_index_H") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      result = Quality::Good;

      // test it
      if (h->GetBinContent(401) == 0) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string BasicDigitQcCheck::getAcceptedType() { return "TH1"; }

void BasicDigitQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "ChipHitMaps/mMFT_chip_index_H") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetLineColor(kGreen + 2);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h->SetLineColor(kRed + 1);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      h->SetLineColor(kOrange);
    }
    // h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::mft
