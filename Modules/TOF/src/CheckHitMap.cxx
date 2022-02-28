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
/// \file   CheckHitMap.cxx
/// \author Nicolò Jacazio nicolo.jacazio@cern.ch
/// \brief  Checker for the hit map hit obtained with the TaskDigits
///

// QC
#include "TOF/CheckHitMap.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckHitMap::configure()
{
  mPhosModuleMessage.configure(13.f, 38.f, 16.f, 53.f); // Values corresponding to the PHOS hole
  mPhosModuleMessage.configureEnabledFlag(mCustomParameters);
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckHitMap::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "TOFRawHitMap") {
      auto* h = dynamic_cast<TH1I*>(mo->getObject());
      if (h->GetEntries() == 0) { // Histogram is empty
        result = Quality::Medium;
        mShifterMessages.AddMessage("No counts!");
      } else { // Histogram is non empty. Here we should check that it is in agreement with the reference from CCDB -> TODO
      }
    }
  }
  return result;
}

std::string CheckHitMap::getAcceptedType() { return "TH2F"; }

void CheckHitMap::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "TOFRawHitMap") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    // auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    // if (!msg) {
    //   return;
    // }
    auto msgPhos = mPhosModuleMessage.MakeMessagePad(h, checkResult, "bl");
    if (!msgPhos) {
      return;
    }
    msgPhos->AddText("PHOS");
    // if (checkResult == Quality::Good) {
    //   msg->AddText(Form("Mean value = %5.2f", mRawHitsMean));
    //   msg->AddText(Form("Reference range: %5.2f-%5.2f", mMinRawHits, mMaxRawHits));
    //   msg->AddText(Form("Events with 0 hits = %5.2f%%", mRawHitsZeroMultIntegral * 100. / mRawHitsIntegral));
    //   msg->AddText("OK!");
    // } else if (checkResult == Quality::Bad) {
    //   msg->AddText("Call TOF on-call.");
    // } else if (checkResult == Quality::Medium) {
    //   ILOG(Info, Support) << "Quality::medium, setting to yellow";
    //   msg->AddText("IF TOF IN RUN email TOF on-call.");
    // }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
  }
}

} // namespace o2::quality_control_modules::tof
