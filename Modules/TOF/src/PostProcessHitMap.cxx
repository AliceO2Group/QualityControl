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

// QualityControl includes
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "TOF/PostProcessHitMap.h"
#include "QualityControl/DatabaseInterface.h"

// ROOT includes
#include <TH2F.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TColor.h>
#include <boost/property_tree/ptree.hpp>

using namespace std;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::tof
{

void PostProcessHitMap::configure(const boost::property_tree::ptree& config)
{
  const std::string baseJsonPath = "qc.postprocessing." + getID() + ".customization.";
  mCCDBPath = config.get<std::string>(baseJsonPath + "CCDBPath", "TOF/MO/TaskDigits/");
  ILOG(Info, Support) << "Setting CCDBPath to " << mCCDBPath << ENDM;
  mCCDBPathObject = config.get<std::string>(baseJsonPath + "CCDBPathObject", "HitMapNoiseFiltered");
  ILOG(Info, Support) << "Setting CCDBPathObject to " << mCCDBPathObject << ENDM;
  mRefMapCcdbPath = config.get<std::string>(baseJsonPath + "RefMapCcdbPath", "/TOF/Calib/FEELIGHT");
  ILOG(Info, Support) << "Setting RefMapCcdbPath to " << mRefMapCcdbPath << ENDM;
  mRefMapTimestamp = config.get<int>(baseJsonPath + "RefMapTimestamp", -1);
  ILOG(Info, Support) << "Setting RefMapTimestamp to " << mRefMapTimestamp << ENDM;
  mDrawRefOnTop = config.get<bool>(baseJsonPath + "DrawRefOnTop", false);
  ILOG(Info, Support) << "Setting DrawRefOnTop to " << (mDrawRefOnTop ? "true" : "false") << ENDM;
}

void PostProcessHitMap::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // Setting up services
  mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
}

void PostProcessHitMap::update(Trigger t, framework::ServiceRegistryRef services)
{
  // Getting the hit map
  const auto mo = mDatabase->retrieveMO(mCCDBPath, mCCDBPathObject, t.timestamp, t.activity);
  TH2F* h = static_cast<TH2F*>(mo ? mo->getObject() : nullptr);
  if (!h) {
    ILOG(Warning, Devel) << "MO '" << mCCDBPathObject << "' not found in path " << mCCDBPath << " with timestamp " << t.timestamp << ENDM;
    return;
  }
  // Getting the reference map
  const o2::tof::TOFFEElightInfo* refmap = nullptr;
  refmap = o2::quality_control::core::UserCodeInterface::retrieveConditionAny<o2::tof::TOFFEElightInfo>(mRefMapCcdbPath);
  if (!mHistoRefHitMap) {
    ILOG(Debug, Devel) << "making new refmap from " << mRefMapCcdbPath << " and timestamp " << mRefMapTimestamp << ENDM;
    mHistoRefHitMap.reset(static_cast<TH2F*>(h->Clone("ReferenceHitMap")));
    mHistoRefHitMap->SetTitle(Form("%s (With reference)", mHistoRefHitMap->GetTitle()));
    mHistoRefHitMap->SetFillColor(kRed);
    if (mDrawRefOnTop) {
      mHistoRefHitMap->SetFillColor(kBlue);
    }
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

  if (!mHistoHitMap) {
    ILOG(Debug, Devel) << "making new hit map" << ENDM;
    mHistoHitMap.reset(static_cast<TH2F*>(h->Clone("WithReferenceHitMap")));
    mHistoHitMap->SetFillColor(kGreen);
    if (mDrawRefOnTop) {
      mHistoHitMap->SetFillColor(kRed);
    }
  }
  mHistoHitMap->Reset();
  for (int i = 1; i <= h->GetNbinsX(); i++) {
    for (int j = 1; j <= h->GetNbinsY(); j++) {
      mHistoHitMap->SetBinContent(i, j, 0);
      if (h->GetBinContent(i, j) > 0) {
        mHistoHitMap->SetBinContent(i, j, 1);
      }
    }
  }

  // Finalizing the plot and updating the MO on the CCDB
  if (!mHistoRefHitMap) {
    ILOG(Warning, Support) << "mHistoRefHitMap undefined, can't finalize" << ENDM;
    return;
  }
  if (!mHistoHitMap) {
    ILOG(Warning, Support) << "mHistoHitMap undefined, can't finalize" << ENDM;
    return;
  }
  TCanvas* canvas = new TCanvas(mHistoHitMap->GetName(), mHistoHitMap->GetName());
  mHistoRefHitMap->GetZaxis()->SetRangeUser(0, 1);
  mHistoHitMap->GetZaxis()->SetRangeUser(0, 1);

  if (!mDrawRefOnTop) {
    mHistoRefHitMap->GetListOfFunctions()->Clear();
    mHistoRefHitMap->Draw("BOX");
    mHistoHitMap->GetListOfFunctions()->Clear();
    mHistoHitMap->Draw("BOXsame");

  } else {
    mHistoHitMap->GetListOfFunctions()->Clear();
    mHistoHitMap->Draw("BOX");
    mHistoRefHitMap->GetListOfFunctions()->Clear();
    mHistoRefHitMap->Draw("BOXsame");
  }
  TPaveText phosPad{ 13.f, 38.f, 16.f, 53.f, "bl" };
  phosPad.SetTextSize(0.05);
  phosPad.SetBorderSize(1);
  phosPad.SetTextColor(kBlack);
  phosPad.SetFillColor(kGreen);
  phosPad.SetFillStyle(3004);
  phosPad.AddText("Red: No Match");
  phosPad.AddText("Blu: RefMap");
  phosPad.AddText("Green: HitMap");
  phosPad.Draw();

  // Draw the shifter message
  if (mHistoHitMap->GetListOfFunctions()->FindObject(Form("%s_msg", mHistoHitMap->GetName()))) {
    mHistoHitMap->GetListOfFunctions()->FindObject(Form("%s_msg", mHistoHitMap->GetName()))->Draw("same");
  }

  auto moToStore = std::make_shared<o2::quality_control::core::MonitorObject>(canvas, getID(), "o2::quality_control_modules::tof::PostProcessDiagnosticPerCreate", "TOF");
  moToStore->setIsOwner(false);
  mDatabase->storeMO(moToStore);
  // It should delete everything inside. Confirmed by trying to delete histo after and getting a segfault.
  delete canvas;
}

} // namespace o2::quality_control_modules::tof
