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
#include "QualityControl/MonitorObject.h"
#include "CCDB/BasicCCDBManager.h"
#include "DataFormatsTOF/TOFFEElightInfo.h"
#include "TOFBase/Geo.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckHitMap::configure()
{
  utils::parseBooleanParameter(mCustomParameters, "EnableReferenceHitMap", mEnableReferenceHitMap);
  utils::parseStrParameter(mCustomParameters, "RefMapCcdbPath", mRefMapCcdbPath);
  utils::parseIntParameter(mCustomParameters, "RefMapTimestamp", mRefMapTimestamp);
  utils::parseIntParameter(mCustomParameters, "MaxHitMoreThanRef", mMaxHitMoreThanRef);
  utils::parseIntParameter(mCustomParameters, "MaxRefMoreThanHit", mMaxRefMoreThanHit); /// by defoult it is to 317 = 5 % of the total enabled channels
  utils::parseBooleanParameter(mCustomParameters, "EnablePadPerMismatch", mEnablePadPerMismatch);
  mPhosModuleMessage.clearQualityMessages();
  mPhosModuleMessage.configureEnabledFlag(mCustomParameters);
  mShifterMessages.mMessageWhenBad = "Call TOF on-call";
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckHitMap::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {ILOG(Debug, Devel) << "Checking " << mo->getName() << ENDM;
    const auto* h = static_cast<TH2F*>(mo->getObject());
    if (h->GetEntries() == 0) { // Histogram is empty
      result = Quality::Medium;
      mShifterMessages.AddMessage("No counts!");
    } else if (mEnableReferenceHitMap) { // Histogram is non empty. Here we should check that it is in agreement with the reference from CCDB
      // Getting the reference map
      const auto* refmap = o2::ccdb::BasicCCDBManager::instance().getForTimeStamp<o2::tof::TOFFEElightInfo>(mRefMapCcdbPath, mRefMapTimestamp);
      if (!mHistoRefHitMap) { // Check that binning is compatible with Monitoring Object?
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
      }

      mNWithHits = 0;
      mNEnabled = 0;
      mHitMoreThanRef.clear();
      mRefMoreThanHit.clear();
      bool hasHit = false;
      bool hasRef = false;
      for (int i = 1; i <= h->GetNbinsX(); i++) {
        for (int j = 1; j <= h->GetNbinsY(); j++) {
          hasHit = false;
          hasRef = false;
          if (h->GetBinContent(i, j) > 0.0) { // Yes hit
            mNWithHits++;
            hasHit = true;
          }
          if (mHistoRefHitMap->GetBinContent(i, j) > 0.0) { // Ch. enabled
            mNEnabled++;
            hasRef = true;
          }
          if (hasHit && !hasRef) {
            mHitMoreThanRef.push_back(std::pair<int, int>(i, j));
          }
          if (!hasHit && hasRef) {
            mRefMoreThanHit.push_back(std::pair<int, int>(i, j));
          }
        }
      }

      result = Quality::Good;
      const auto mNMishmatching_HitMoreThanRef = mHitMoreThanRef.size();
      const auto mNMishmatching_RefMoreThanHit = mRefMoreThanHit.size();
      const auto mNMishmatching = mRefMoreThanHit.size() + mHitMoreThanRef.size();
      if (mNMishmatching_HitMoreThanRef > mMaxHitMoreThanRef || mNMishmatching_RefMoreThanHit > mMaxRefMoreThanHit) {
        result = Quality::Bad;
        mShifterMessages.AddMessage(Form("Hits %i, enabled %i, %zu total Mismatch", mNWithHits, mNEnabled, mNMishmatching));
        if (mNMishmatching_HitMoreThanRef > mMaxHitMoreThanRef) {
          mShifterMessages.AddMessage(Form("%zu has hit but not ref > %i Thr.", mNMishmatching_HitMoreThanRef, mMaxHitMoreThanRef));
        }
        if (mNMishmatching_RefMoreThanHit > mMaxRefMoreThanHit) {
          mShifterMessages.AddMessage(Form("%zu has ref but not hit > %i Thr.", mNMishmatching_RefMoreThanHit, mMaxRefMoreThanHit));
        }
      }

      return result;
    }
  }
  return result;
}

void CheckHitMap::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Debug, Devel) << "Beautifying " << mo->getName() << ENDM;if (1) {
    auto* h = static_cast<TH2F*>(mo->getObject());
    if (checkResult != Quality::Good) {
      auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    }
    auto msgPhos = mPhosModuleMessage.MakeMessagePad(h, Quality::Good, "bl");
    if (!msgPhos) {
      return;
    }
    msgPhos->SetFillStyle(3004);
    msgPhos->SetTextSize(30);
    msgPhos->AddText("PHOS");
    if (mEnablePadPerMismatch) {             // Adding pads to highlight mismatches
      for (const auto p : mHitMoreThanRef) { // Pads for when hits are more than the reference map
        float xl = h->GetXaxis()->GetBinLowEdge(p.first);
        float xu = h->GetXaxis()->GetBinUpEdge(p.first);
        float yl = h->GetYaxis()->GetBinLowEdge(p.second);
        float yu = h->GetYaxis()->GetBinUpEdge(p.second);

        TPaveText* pad = new TPaveText(xl, yl, xu, yu);

        pad->SetName(Form("hits more than ref_%i_%i", p.first, p.second));
        h->GetListOfFunctions()->Add(pad);
        pad->SetBorderSize(1);
        pad->SetFillColor(kMagenta);
        pad->AddText(" ");

        ILOG(Warning, Support) << "Adding extra pad to " << mo->GetName() << " in position along x: " << xl << " - " << xu << " and y: " << yl << " - " << yu << ENDM;
      }

      for (const auto p : mRefMoreThanHit) { // Pads for when hits are less than the reference map
        float xl = h->GetXaxis()->GetBinLowEdge(p.first);
        float xu = h->GetXaxis()->GetBinUpEdge(p.first);
        float yl = h->GetYaxis()->GetBinLowEdge(p.second);
        float yu = h->GetYaxis()->GetBinUpEdge(p.second);

        TPaveText* pad = new TPaveText(xl, yl, xu, yu);

        pad->SetName(Form("ref more than hit_%i_%i", p.first, p.second));
        h->GetListOfFunctions()->Add(pad);
        pad->SetBorderSize(1);
        pad->SetFillColor(kRed);
        // pad->SetFillStyle(3004);
        pad->AddText(" ");

        ILOG(Warning, Support) << "Adding extra pad to " << mo->GetName() << " in position along x: " << xl << " - " << xu << " and y: " << yl << " - " << yu << ENDM;
      }
    }

  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
    return;
  }
}

} // namespace o2::quality_control_modules::tof
