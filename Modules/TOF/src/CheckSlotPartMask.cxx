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
/// \file   CheckSlotPartMask.cxx
/// \author Francesca Ercolessi
/// \brief  Checker for slot partecipating
///

// QC
#include "TOF/CheckSlotPartMask.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "TOF/Utils.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckSlotPartMask::configure()
{
  utils::parseIntParameter(mCustomParameters, "NCrates", mNCrates);
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckSlotPartMask::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking slot partecipating" << ENDM;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "hSlotPartMask") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      double hitsxcrate[72] = {};
      int ncrate = 0;
      for (int xbin = 1; xbin <= h->GetNbinsX(); xbin++) {   // loop over crates
        for (int ybin = 1; ybin <= h->GetNbinsY(); ybin++) { // loop over slots
          hitsxcrate[xbin] += h->GetBinContent(xbin, ybin);
        }
        if (hitsxcrate[xbin] == 0) { // is entire crate empty?
          ncrate++;
        }
      }
      if (ncrate > mNCrates) {
        result = Quality::Bad;
      } else {
        result = Quality::Good;
      }
    }
  }
  return result;
}

std::string CheckSlotPartMask::getAcceptedType() { return "TH2F"; }

void CheckSlotPartMask::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "hSlotPartMask") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult, "bl");
    if (!msg) {
      return;
    }
    if (checkResult == Quality::Good) {
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Many links missing.");
      msg->AddText("Call TOF on-call.");
    } else if (checkResult == Quality::Medium) {
      // msg->AddText("");
      // msg->AddText("");
    }
  } else
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
}

} // namespace o2::quality_control_modules::tof
