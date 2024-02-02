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
/// \file   CheckLostOrbits.cxx
/// \author Francesca Ercolessi
/// \brief  Checker for lost orbits
///

// QC
#include "TOF/CheckLostOrbits.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "TOF/Utils.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckLostOrbits::configure()
{
  mShifterMessages.configure(mCustomParameters);

  utils::parseDoubleParameter(mCustomParameters, "FractionThr", mFractionThr);
}

Quality CheckLostOrbits::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking fraction of lost orbits" << ENDM;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "OrbitsInTFEfficiency") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      if (h->GetBinCenter((h->GetMaximumBin())) > mFractionThr) {
        result = Quality::Good;
      } else {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string CheckLostOrbits::getAcceptedType() { return "TH2F"; }

void CheckLostOrbits::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "OrbitsInTFEfficiency") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult, "NDC");
    if (!msg) {
      return;
    }
    msg->AddText(Form("Max peak position = %.3f", h->GetBinCenter(h->GetMaximumBin())));
    msg->AddText(Form("Mean = %.3f", h->GetMean()));
  }
}

} // namespace o2::quality_control_modules::tof
