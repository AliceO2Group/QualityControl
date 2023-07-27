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
/// \file   PulseHeightTrackMatch.cxx
/// \author Vikash Sumberia
///

#include <TLine.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include "DataFormatsTRD/TrackTRD.h"
#include "QualityControl/QcInfoLogger.h"
#include "TRD/PulseHeightTrackMatch.h"
#include "TRDCalibration/PulseHeight.h"
#include "DataFormatsTRD/PHData.h"
#include "TRDWorkflowIO/TRDCalibWriterSpec.h"
#include "DataFormatsGlobalTracking/RecoContainer.h"
#include <Framework/InputRecord.h>
#include "CCDB/BasicCCDBManager.h"
namespace o2::quality_control_modules::trd
{
PulseHeightTrackMatch::~PulseHeightTrackMatch()
{
}

void PulseHeightTrackMatch::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
}

void PulseHeightTrackMatch::buildHistograms()
{
  mPulseHeightpro.reset(new TProfile("PulseHeight/mPulseHeightpro", "PulseHeight;Timebins;ADC Counts", 30, -0.5, 29.5));
  drawLinesOnPulseHeight(mPulseHeightpro.get());
  mPulseHeightpro.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightpro.get());

  mPulseHeightperchamber.reset(new TProfile2D("PulseHeight/mPulseHeightperchamber", "PulseHeight per chamber;Timebin;Chamber", 30, -0.5, 29.5, 540, 0, 540));
  mPulseHeightperchamber.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightperchamber.get());
  getObjectsManager()->setDefaultDrawOptions(mPulseHeightperchamber.get()->GetName(), "colz");
}

void PulseHeightTrackMatch::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TRDPulseHeightQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // values configurable from json
  if (auto param = mCustomParameters.find("driftregionstart"); param != mCustomParameters.end()) {
    mDriftRegion.first = stof(param->second);
    ILOG(Debug, Devel) << "configure() : using peakregionstart = " << mDriftRegion.first << ENDM;
  } else {
    mDriftRegion.first = 7.0;
    ILOG(Debug, Devel) << "configure() : using default dritfregionstart = " << mDriftRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("driftregionend"); param != mCustomParameters.end()) {
    mDriftRegion.second = stof(param->second);
    ILOG(Debug, Devel) << "configure() : using peakregionstart = " << mDriftRegion.second << ENDM;
  } else {
    mDriftRegion.second = 20.0;
    ILOG(Debug, Devel) << "configure() : using default dritfregionstart = " << mDriftRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeaklower"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.first = stof(param->second);
    ILOG(Debug, Devel) << "configure() : using pulsehheightlower	= " << mPulseHeightPeakRegion.first << ENDM;
  } else {
    mPulseHeightPeakRegion.first = 0.0;
    ILOG(Debug, Devel) << "configure() : using default pulseheightlower = " << mPulseHeightPeakRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeakupper"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.second = stof(param->second);
    ILOG(Debug, Devel) << "configure() : using pulsehheightupper	= " << mPulseHeightPeakRegion.second << ENDM;
  } else {
    mPulseHeightPeakRegion.second = 5.0;
    ILOG(Debug, Devel) << "configure() : using default pulseheightupper = " << mPulseHeightPeakRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("trackType"); param != mCustomParameters.end()) {
    mTrackType = std::stof(param->second);
    if (mTrackType == 0x1) {
      ILOG(Debug, Devel) << "configure() : using trackType	= " << mTrackType << " ITS-TPC-TRD tracks" << ENDM;
    }
    if (mTrackType == 0x2) {
      ILOG(Debug, Devel) << "configure() : using trackType	= " << mTrackType << " TPC-TRD tracks" << ENDM;
    }
  } else {
    mTrackType = 0xf;
    ILOG(Debug, Devel) << "configure() : using default trackType = " << mTrackType << " all the tracks" << ENDM;
  }

  retrieveCCDBSettings();
  buildHistograms();
}

void PulseHeightTrackMatch::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
} // set stats/stacs

void PulseHeightTrackMatch::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void PulseHeightTrackMatch::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto phDataArr = ctx.inputs().get<gsl::span<o2::trd::PHData>>("phValues");

  for (const auto& phData : phDataArr) {
    if (mTrackType[phData.getType()]) {
      mPulseHeightpro->Fill(phData.getTimebin(), phData.getADC());
      mPulseHeightperchamber->Fill(phData.getTimebin(), phData.getDetector(), phData.getADC());
    }
  }
}

void PulseHeightTrackMatch::drawLinesOnPulseHeight(TProfile* h)
{
  TLine* lmin = new TLine(mPulseHeightPeakRegion.first, -10, mPulseHeightPeakRegion.first, 1e9);
  TLine* lmax = new TLine(mPulseHeightPeakRegion.second, -10, mPulseHeightPeakRegion.second, 1e9);
  lmin->SetLineStyle(2);
  lmax->SetLineStyle(2);
  lmin->SetLineColor(kRed);
  lmax->SetLineColor(kRed);
  h->GetListOfFunctions()->Add(lmin);
  h->GetListOfFunctions()->Add(lmax);
}

void PulseHeightTrackMatch::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void PulseHeightTrackMatch::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void PulseHeightTrackMatch::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histogram" << ENDM;
  mPulseHeightpro->Reset();
  mPulseHeightperchamber->Reset();
}
} // namespace o2::quality_control_modules::trd
