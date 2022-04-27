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
#include "TOF/Utils.h"
#include "QualityControl/QcInfoLogger.h"
#include "CCDB/BasicCCDBManager.h"
#include "DataFormatsTOF/TOFFEElightInfo.h"
#include "TOFBase/Geo.h"

// ROOT
#include "TLine.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckHitMap::configure()
{
  utils::parseBooleanParameter(mCustomParameters, "EnableReferenceHitMap", mEnableReferenceHitMap);
  utils::parseStrParameter(mCustomParameters, "RefMapCcdbPath", mRefMapCcdbPath);
  utils::parseIntParameter(mCustomParameters, "RefMapTimestamp", mRefMapTimestamp);
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
    if (mo->getName() != mAcceptedName) {
      ILOG(Error, Support) << "Cannot check MO " << mo->getName() << " " << moName << " which does not have name " << mAcceptedName << ENDM;
      continue;
    }
    ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;
    const auto* h = static_cast<TH2F*>(mo->getObject());
    if (h->GetEntries() == 0) { // Histogram is empty
      result = Quality::Medium;
      mShifterMessages.AddMessage("No counts!");
    } else if (mEnableReferenceHitMap) { // Histogram is non empty. Here we should check that it is in agreement with the reference from CCDB
      // Getting the reference map
      const auto* refmap = o2::ccdb::BasicCCDBManager::instance().getForTimeStamp<o2::tof::TOFFEElightInfo>(mRefMapCcdbPath, mRefMapTimestamp);
      if (!mHistoRefHitMap) {
        ILOG(Debug, Devel) << "making new refmap " << refmap << ENDM;
        mHistoRefHitMap.reset(static_cast<TH2F*>(h->Clone("ReferenceHitMap")));
      }
      mHistoRefHitMap->Reset();
      int det[5] = { 0 }; // Coordinates
      int strip = 0;      // Strip

      for (auto i = 0; i < refmap->NCHANNELS; i++) {
        if (!refmap->getChannelEnabled(i)) {
          continue;
        }
        o2::tof::Geo::getVolumeIndices(refmap->getChannelEnabled(i), det);
        strip = o2::tof::Geo::getStripNumberPerSM(det[1], det[2]); // Strip index in the SM
        mHistoRefHitMap->SetBinContent(strip, det[0] * 4 + det[4] / 12, 1);
      }
      h->GetListOfFunctions()->Add(mHistoRefHitMap.get());

      // for (int i = 1; i <= mHistoRefHitMap->GetNbinsX(); i++) {
      //   for (int j = 1; j <= mHistoRefHitMap->GetNbinsY(); j++) {
      //     mHistoRefHitMap->SetBinContent(i, i, 0);
      //   }
      // }
      ILOG(Debug, Devel) << "got refmap " << refmap << ENDM;
      result = Quality::Good;
      for (int i = 1; i <= h->GetNbinsX(); i++) {
        for (int j = 1; j <= h->GetNbinsY(); j++) {
          if (0) {
            result = Quality::Medium;
          }
        }
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
  if (mo->getName() == mAcceptedName) {
    auto* h = static_cast<TH2F*>(mo->getObject());
    if (checkResult != Quality::Good) {
      auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    }
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
    //   ILOG(Info, Support) << "Quality::medium, setting to yellow" << ENDM;
    //   msg->AddText("IF TOF IN RUN email TOF on-call.");
    // }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
