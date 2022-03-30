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
  mPhosModuleMessage.configureEnabledFlag(mCustomParameters);
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckHitMap::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    if (!isObjectCheckable(mo)) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which is not of type " << getAcceptedType() << ENDM;
      continue;
    }
    ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;
    if (mo->getName() == "HitMap") {
      const auto* h = static_cast<TH2F*>(mo->getObject());
      if (h->GetEntries() == 0) { // Histogram is empty
        result = Quality::Medium;
        mShifterMessages.AddMessage("No counts!");
      } else { // Histogram is non empty. Here we should check that it is in agreement with the reference from CCDB -> TODO
      }
    }
  }
  return result;
}

void CheckHitMap::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Debug, Devel) << "Beautifying " << mo->getName() << ENDM;
  if (!isObjectCheckable(mo)) {
    ILOG(Error, Support) << "Cannot beautify MO " << mo->getName() << " which is not of type " << getAcceptedType() << ENDM;
    return;
  }
  if (mo->getName() == "TOFRawHitMap") {
    auto* h = static_cast<TH2F*>(mo->getObject());
    // auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    // if (!msg) {
    //   return;
    // }
    auto msgPhos = mPhosModuleMessage.MakeMessagePad(h, Quality::Good, "bl");
    if (!msgPhos) {
      return;
    }
    msgPhos->SetFillStyle(3004);
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
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
