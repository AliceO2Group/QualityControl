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
/// \file   Tracks.cxx
/// \author Stefan Heckel, sheckel@cern.ch
///

// root includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
//#include <gsl/span>

// O2 includes
#include "Framework/ProcessingContext.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "TPCQC/Helpers.h"
#include <Framework/InputRecord.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/Tracks.h"

namespace o2::quality_control_modules::tpc
{

void Tracks::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TPC Tracks QC task" << ENDM;

  mQCTracks.initializeHistograms();
  o2::tpc::qc::helpers::setStyleHistogram2D(mQCTracks.getHistograms2D());

  for (auto& hist : mQCTracks.getHistograms1D()) {
    getObjectsManager()->startPublishing(&hist);
  }

  for (auto& hist2 : mQCTracks.getHistograms2D()) {
    getObjectsManager()->startPublishing(&hist2);
  }

  for (auto& histR : mQCTracks.getHistogramRatios1D()) {
    getObjectsManager()->startPublishing(&histR);
  }
}

void Tracks::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mQCTracks.resetHistograms();
}

void Tracks::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void Tracks::monitorData(o2::framework::ProcessingContext& ctx)
{
  using TrackType = std::vector<o2::tpc::TrackTPC>;
  auto tracks = ctx.inputs().get<TrackType>("inputTracks");
  //using TracksType = gsl::span<o2::tpc::TrackTPC>;
  //const auto tracks = ctx.inputs().get<TracksType>("inputTracks");
  //ILOG(Info, Support) << "monitorData: " << tracks.size() << ENDM;

  for (auto const& track : tracks) {
    mQCTracks.processTrack(track);
  }
  //mQCTracks.processAllTracks(tracks);
}

void Tracks::endOfCycle()
{
  mQCTracks.processEndOfCycle();
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void Tracks::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void Tracks::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mQCTracks.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
