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
/// \file   PostProcessHitMap.cxx
/// \brief  Post processing to produce a plot of the TOF hit map with the reference enabled channels
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \since  09/07/2022
///

// O2 includes
#include "DataFormatsTOF/TOFFEElightInfo.h"
#include "TOFBase/Geo.h"
#include "CCDB/BasicCCDBManager.h"

// QC includes
#include "TOF/PostProcessHitMap.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT includes
#include <TH2F.h>
#include <TCanvas.h>
#include <TPaveText.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tof
{

void PostProcessHitMap::configure(const boost::property_tree::ptree& config)
{
  mRefMapCcdbPath = "/TOF/Calib/FEELIGHT";
  mCCDBPath = "TOF/MO/TaskDigits/";
  if (const auto& customConfigs = config.get_child_optional("qc.postprocessing." + getID() + ".customization"); customConfigs.has_value()) {
    for (const auto& customConfig : customConfigs.value()) { // Plot configuration
      if (const auto& customNames = customConfig.second.get_child_optional("name"); customNames.has_value()) {
        if (customConfig.second.get<std::string>("name") == "CCDBPath") {
          mCCDBPath = customConfig.second.get<std::string>("value");
          ILOG(Info, Support) << "Setting CCDBPath to " << mCCDBPath << ENDM;
        } else if (customConfig.second.get<std::string>("name") == "RefMapCcdbPath") {
          mRefMapCcdbPath = customConfig.second.get<std::string>("value");
          ILOG(Info, Support) << "Setting RefMapCcdbPath to " << mRefMapCcdbPath << ENDM;
        } else if (customConfig.second.get<std::string>("name") == "RefMapTimestamp") {
          mRefMapTimestamp = customConfig.second.get<int>("value");
          ILOG(Info, Support) << "Setting RefMapTimestamp to " << mRefMapTimestamp << ENDM;
        }
      }
    }
  }
}

void PostProcessHitMap::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // Setting up services
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
}

void PostProcessHitMap::update(Trigger t, framework::ServiceRegistryRef)
{
  // Getting the hit map
  const auto mo = mDatabase->retrieveMO(mCCDBPath, "HitMap", t.timestamp, t.activity);
  TH2F* h = static_cast<TH2F*>(mo ? mo->getObject() : nullptr);
  if (!h) {
    ILOG(Warning, Devel) << "MO 'HitMap' not found in path " << mCCDBPath << " with timestamp " << t.timestamp << ENDM;
    return;
  }

  // Getting the reference map
  const auto* refmap = o2::ccdb::BasicCCDBManager::instance().getForTimeStamp<o2::tof::TOFFEElightInfo>(mRefMapCcdbPath, mRefMapTimestamp);
  if (!mHistoRefHitMap) {
    ILOG(Debug, Devel) << "making new refmap from " << mRefMapCcdbPath << " and timestamp " << mRefMapTimestamp << ENDM;
    mHistoRefHitMap.reset(static_cast<TH2F*>(h->Clone("ReferenceHitMap")));
    mHistoRefHitMap->SetTitle(Form("%s (With reference)", mHistoRefHitMap->GetTitle()));
    mHistoRefHitMap->SetFillColor(2);
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

  if (!mHistoHitMap) {
    ILOG(Debug, Devel) << "making new hit map" << ENDM;
    mHistoHitMap.reset(static_cast<TH2F*>(h->Clone("WithReferenceHitMap")));
    mHistoHitMap->SetFillColor(3);
  }
  mNWithHits = 0;
  mNEnabled = 0;
  for (int i = 1; i <= mHistoHitMap->GetNbinsX(); i++) {
    for (int j = 1; j <= mHistoHitMap->GetNbinsY(); j++) {
      mHistoHitMap->SetBinContent(i, j, h->GetBinContent(i, j));
      // Check matching with respect to reference
      if (h->GetBinContent(i, j) > 0.0) { // Yes hit
        mNWithHits++;
      }
      if (mHistoRefHitMap->GetBinContent(i, j) > 0.0) { // Ch. enabled
        mNEnabled++;
      }
    }
  }
}

void PostProcessHitMap::finalize(Trigger t, framework::ServiceRegistryRef)
{
  if (!mHistoRefHitMap) {
    ILOG(Warning, Support) << "mHistoRefHitMap undefined, can't finalize" << ENDM;
    return;
  }
  if (!mHistoHitMap) {
    ILOG(Warning, Support) << "mHistoHitMap undefined, can't finalize" << ENDM;
    return;
  }
  TCanvas* c = new TCanvas(mHistoHitMap->GetName(), mHistoHitMap->GetName());
  mHistoRefHitMap->Draw("BOX");
  mHistoHitMap->Draw("BOXsame");
  TPaveText phosPad{ 13.f, 38.f, 16.f, 53.f, "bl" };
  phosPad.SetTextSize(0.05);
  phosPad.SetBorderSize(1);
  phosPad.SetTextColor(kBlack);
  phosPad.SetFillColor(kGreen);
  phosPad.SetFillStyle(3004);
  phosPad.AddText("Red: Channel ON");
  phosPad.AddText("Green: Hits");
  phosPad.Draw();
  TPaveText errorPad{ 0.9f, 0.1f, 0.99f, 0.99f, "blNDC" };
  errorPad.SetTextSize(0.05);
  errorPad.SetBorderSize(1);
  errorPad.SetTextColor(kBlack);
  errorPad.SetFillColor(kRed);
  if ((mNWithHits - mNEnabled) > mTrheshold) {
    errorPad.AddText(Form("Hits %i > enabled %i", mNWithHits, mNEnabled));
    errorPad.Draw();
  } else if ((mNWithHits - mNEnabled) < mTrheshold) {
    errorPad.AddText(Form("Hits %i < enabled %i", mNWithHits, mNEnabled));
    errorPad.Draw();
  }

  auto mo = std::make_shared<o2::quality_control::core::MonitorObject>(c, "PostProcessHitMap", "o2::quality_control_modules::tof::PostProcessDiagnosticPerCreate", "TOF");
  mo->setIsOwner(false);
  mDatabase->storeMO(mo);

  // It should delete everything inside. Confirmed by trying to delete histo after and getting a segfault.
  delete c;
}

} // namespace o2::quality_control_modules::tof
