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

void CheckHitMap::configure(string /*name*/)
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
        o2::tof::Geo::getVolumeIndices(i, det);
        strip = o2::tof::Geo::getStripNumberPerSM(det[1], det[2]); // Strip index in the SM
        const int istrip = mHistoRefHitMap->GetYaxis()->FindBin(strip);
        const int crate = det[0] * 4 + det[4] / 12;
        const int icrate = mHistoRefHitMap->GetXaxis()->FindBin(0.25f * crate);
        mHistoRefHitMap->SetBinContent(icrate, istrip, 1);
        // ILOG(Debug, Devel) << "setting channel " << i << " as enabled: crate " << crate << " -> " << icrate << "/" << mHistoRefHitMap->GetNbinsX() << ", strip " << strip << " -> " << istrip << "/" << mHistoRefHitMap->GetNbinsY() << ENDM;
      }
      mNWithHits = 0;
      mNEnabled = 0;
      for (int i = 1; i <= h->GetNbinsX(); i++) {
        for (int j = 1; j <= h->GetNbinsY(); j++) {
          if (h->GetBinContent(i, j) > 0.0) { // Yes hit
            mNWithHits++;
          }
          if (mHistoRefHitMap->GetBinContent(i, j) > 0.0) { // Ch. enabled
            mNEnabled++;
          }
        }
      }

      if ((mNWithHits - mNEnabled) > mTrheshold) {
        mShifterMessages.AddMessage(Form("Hits %i > enabled %i (Thr. %i)", mNWithHits, mNEnabled, mTrheshold));
      } else if ((mNWithHits - mNEnabled) < mTrheshold) {
        mShifterMessages.AddMessage(Form("Hits %i < enabled %i (Thr. %i)", mNWithHits, mNEnabled, mTrheshold));
      }
      result = Quality::Bad;
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
      if (checkResult == Quality::Bad) {
        msg->AddText("Call TOF on-call.");
      } else if (checkResult == Quality::Medium) {
        msg->AddText("IF TOF IN RUN email TOF on-call.");
      }
    }
    auto msgPhos = mPhosModuleMessage.MakeMessagePad(h, Quality::Good, "bl");
    if (!msgPhos) {
      return;
    }
    msgPhos->SetFillStyle(3004);
    msgPhos->AddText("PHOS");
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
