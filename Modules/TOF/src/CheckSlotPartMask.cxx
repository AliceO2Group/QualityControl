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
  utils::parseIntParameter(mCustomParameters, "MinNhitsPerSlot", mMinNhitsPerSlot);
  utils::parseIntParameter(mCustomParameters, "MaxNumberIneffientSlotPerCrate", mMaxNumberIneffientSlotPerCrate);
  utils::parseDoubleParameter(mCustomParameters, "IneffThreshold", mIneffThreshold);
  utils::parseIntParameter(mCustomParameters, "NCrateIneff", mNCrateIneff);

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
      double meanhitsxcrate = 0;
      int ncrate = 0;
      double maxhitsxcrate = hitsxcrate[0];

      for (int xbin = 1; xbin <= h->GetNbinsX(); xbin++) { // loop over crates
        int nslot = 0;
        for (int ybin = 1; ybin <= h->GetNbinsY(); ybin++) { // loop over slot
          hitsxcrate[xbin - 1] += h->GetBinContent(xbin, ybin);
          if (h->GetBinContent(xbin, ybin) <= mMinNhitsPerSlot) {
            nslot++; // number of inefficient slots in each crate
          }
        }
        if (nslot > mMaxNumberIneffientSlotPerCrate) {
          ncrate++; // if the number of inefficient slots in a crate is above a maximun, the crate is inefficient
        }
        if (maxhitsxcrate <= hitsxcrate[xbin - 1])
          maxhitsxcrate = hitsxcrate[xbin - 1]; // Finding the maximum number of hits in a crate
      }
      // look for inefficient crates
      int ncrate_ineff = 0;
      for (int ncrate = 0; ncrate < 72; ncrate++) { // loop over crates
        if (hitsxcrate[ncrate] < mIneffThreshold * maxhitsxcrate) {
          ncrate_ineff++;
        }
      }

      if (ncrate > mNCrates) {
        result = Quality::Bad;
      } else if ((ncrate_ineff > mNCrateIneff)) {
        result = Quality::Medium;
      } else {
        result = Quality::Good;
      }
    }
    return result;
  }
}
std::string CheckSlotPartMask::getAcceptedType() { return "TH2F"; }

void CheckSlotPartMask::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "hSlotPartMask") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult, "bl");
    msg->SetTextSize(30);
    if (!msg) {
      return;
    }
    if (checkResult == Quality::Good) {
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Many crates have many inefficient slots.");
      msg->AddText("Call TOF on-call.");
    } else if (checkResult == Quality::Medium) {
      msg->AddText("Many inefficient crates.");
      // msg->AddText("");
    }
  } else
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
}

} // namespace o2::quality_control_modules::tof
