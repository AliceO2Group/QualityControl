// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TaskCosmics.cxx
/// \author Nicolo' Jacazio
/// \brief  Task to monitor TOF data collected in events from cosmics
///

// ROOT includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH1I.h>
#include <TH2I.h>

// O2 includes
#include "TOFBase/Digit.h"
#include "TOFBase/Geo.h"
#include "DataFormatsTOF/CosmicInfo.h"
#include <Framework/InputRecord.h>

// Fairlogger includes
#include <fairlogger/Logger.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskCosmics.h"

namespace o2::quality_control_modules::tof
{

TaskCosmics::TaskCosmics() : TaskInterface()
{
}

TaskCosmics::~TaskCosmics()
{
  mHistoCrate1.reset();
  mHistoCrate2.reset();
  mHistoCrate1VsCrate2.reset();
  mHistoDeltaT.reset();
  mHistoToT1.reset();
  mHistoToT2.reset();
  mHistoLength.reset();
  mHistoDeltaTLength.reset();
}

void TaskCosmics::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TaskCosmics" << ENDM;

  // Set task parameters from JSON
  if (auto param = mCustomParameters.find("SelDeltaTSignalRegion"); param != mCustomParameters.end()) {
    mSelDeltaTSignalRegion = atoi(param->second.c_str());
    LOG(INFO) << "Set SelDeltaTSignalRegion to " << mSelDeltaTSignalRegion << " ps";
  }
  if (auto param = mCustomParameters.find("SelDeltaTBackgroundRegion"); param != mCustomParameters.end()) {
    mSelDeltaTBackgroundRegion = atoi(param->second.c_str());
    LOG(INFO) << "Set SelDeltaTBackgroundRegion to " << mSelDeltaTBackgroundRegion << " ps";
  }

  mHistoCrate1.reset(new TH1F("Crate1", "Crate1;Crate of first hit;Counts", 72, 0, 72));
  getObjectsManager()->startPublishing(mHistoCrate1.get());
  mHistoCrate2.reset(new TH1F("Crate2", "Crate2;Crate of second hit;Counts", 72, 0, 72));
  getObjectsManager()->startPublishing(mHistoCrate2.get());
  mHistoCrate1VsCrate2.reset(new TH2F("Crate1VsCrate2", "Crate1;Crate of first hit;Crate of second hit;Counts", 72, 0, 72, 72, 0, 72));
  getObjectsManager()->startPublishing(mHistoCrate1VsCrate2.get());
  mHistoDeltaT.reset(new TH1F("DeltaT", "DeltaT;#DeltaT (ps);Counts", 1000, -1e6, 1e6));
  getObjectsManager()->startPublishing(mHistoDeltaT.get());
  mHistoToT1.reset(new TH1F("ToT1", "ToT1;ToT (ns);Counts", 100, 0., 48.8));
  getObjectsManager()->startPublishing(mHistoToT1.get());
  mHistoToT2.reset(new TH1F("ToT2", "ToT2;ToT (ns);Counts", 100, 0., 48.8));
  getObjectsManager()->startPublishing(mHistoToT2.get());
  mHistoLength.reset(new TH1F("Length", "Length;Length (cm);Counts", 100, 500.f, 1000.f));
  getObjectsManager()->startPublishing(mHistoLength.get());
  mHistoDeltaTLength.reset(new TH2F("DeltaTLength", "DeltaT vs Length;Length (cm);#DeltaT (ps);Counts", 100, 500.f, 1000.f, 1000, -1e6, 1e6));
  getObjectsManager()->startPublishing(mHistoDeltaTLength.get());
  mHistoCosmicRate.reset(new TH1F("CosmicRate", "CosmicRate;Crate;Cosmic rate (Hz)", 72, 0., 72));
  mCounterPeak.MakeHistogram(mHistoCosmicRate.get());
  getObjectsManager()->startPublishing(mHistoCosmicRate.get());
}

void TaskCosmics::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  reset();
}

void TaskCosmics::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TaskCosmics::monitorData(o2::framework::ProcessingContext& ctx)
{
  mCounterTF.Count(0);
  const auto cosmics = ctx.inputs().get<std::vector<o2::tof::CosmicInfo>>("infocosmics");
  for (auto i : cosmics) {
    const int crate1 = o2::tof::Geo::getCrateFromECH(o2::tof::Geo::getECHFromCH(i.getCH1()));
    const int crate2 = o2::tof::Geo::getCrateFromECH(o2::tof::Geo::getECHFromCH(i.getCH2()));
    mHistoCrate1->Fill(crate1);
    mHistoCrate2->Fill(crate2);
    mHistoCrate1VsCrate2->Fill(crate1, crate2);
    mHistoDeltaT->Fill(i.getDeltaTime());
    mHistoToT1->Fill(i.getTOT1());
    mHistoToT2->Fill(i.getTOT2());
    mHistoLength->Fill(i.getL());
    mHistoDeltaTLength->Fill(i.getL(), i.getDeltaTime());
    if (abs(i.getDeltaTime()) < mSelDeltaTSignalRegion) {
      mCounterPeak.Count(crate1);
      mCounterPeak.Count(crate2);
    } else if (abs(i.getDeltaTime()) < mSelDeltaTBackgroundRegion) {
      mCounterPeak.Add(crate1, -1);
      mCounterPeak.Add(crate2, -1);
    }
  }
}

void TaskCosmics::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  mCounterPeak.FillHistogram(mHistoCosmicRate.get());
  if (mCounterTF.HowMany(0) > 0) {
    mHistoCosmicRate->Scale(1. / (mCounterTF.HowMany(0) * mTFDuration));
  } else {
    ILOG(Warning, Support) << "TF read are 0" << ENDM;
  }
}

void TaskCosmics::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TaskCosmics::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistoCrate1->Reset();
  mHistoCrate2->Reset();
  mHistoCrate1VsCrate2->Reset();
  mHistoDeltaT->Reset();
  mHistoToT1->Reset();
  mHistoToT2->Reset();
  mHistoLength->Reset();
  mHistoDeltaTLength->Reset();
  mHistoCosmicRate->Reset();
}

} // namespace o2::quality_control_modules::tof
