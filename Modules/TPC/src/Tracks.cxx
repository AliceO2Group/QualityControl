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
  QcInfoLogger::GetInstance() << "initialize TPC Tracks QC task" << AliceO2::InfoLogger::InfoLogger::endm;

  mQCTracks.initializeHistograms();
  o2::tpc::qc::helpers::setStyleHistogram2D(mQCTracks.getHistograms2D());

  for (auto& hist : mQCTracks.getHistograms1D()) {
    getObjectsManager()->startPublishing(&hist);
    getObjectsManager()->addMetadata(hist.GetName(), "custom", "34");
  }

  for (auto& hist2 : mQCTracks.getHistograms2D()) {
    getObjectsManager()->startPublishing(&hist2);
    getObjectsManager()->addMetadata(hist2.GetName(), "custom", "42");
  }

  for (auto& histR : mQCTracks.getHistogramRatios1D()) {
    getObjectsManager()->startPublishing(&histR);
    getObjectsManager()->addMetadata(histR.GetName(), "custom", "51");
  }
}

void Tracks::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mQCTracks.resetHistograms();
}

void Tracks::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void Tracks::monitorData(o2::framework::ProcessingContext& ctx)
{
  using TrackType = std::vector<o2::tpc::TrackTPC>;
  auto tracks = ctx.inputs().get<TrackType>("inputTracks");
  //using TracksType = gsl::span<o2::tpc::TrackTPC>;
  //const auto tracks = ctx.inputs().get<TracksType>("inputTracks");
  //QcInfoLogger::GetInstance() << "monitorData: " << tracks.size() << AliceO2::InfoLogger::InfoLogger::endm;

  for (auto const& track : tracks) {
    mQCTracks.processTrack(track);
  }
  //mQCTracks.processAllTracks(tracks);
}

void Tracks::endOfCycle()
{
  mQCTracks.processEndOfCycle();
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void Tracks::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void Tracks::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mQCTracks.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
