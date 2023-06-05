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
#include "Common/Utils.h"

namespace o2::quality_control_modules::tpc
{

void Tracks::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC Tracks QC task" << ENDM;

  const float cutMindEdxTot = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMindEdxTot");
  const float cutAbsEta = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutAbsEta");
  const int cutMinNCluster = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "cutMinNCluster");

  // set track cuts defaults are (AbsEta = 1.0, nCluster = 60, MindEdxTot  = 20)
  mQCTracks.setTrackCuts(cutAbsEta, cutMinNCluster, cutMindEdxTot);

  mQCTracks.initializeHistograms();
  // pass map of vectors of histograms to be beutified!
  o2::tpc::qc::helpers::setStyleHistogramsInMap(mQCTracks.getMapHist());
  for (auto const& pair : mQCTracks.getMapHist()) {
    getObjectsManager()->startPublishing(pair.second.get());
  }
}

void Tracks::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  mQCTracks.resetHistograms();
}

void Tracks::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void Tracks::monitorData(o2::framework::ProcessingContext& ctx)
{
  using TrackType = std::vector<o2::tpc::TrackTPC>;
  auto tracks = ctx.inputs().get<TrackType>("inputTracks");

  for (auto const& track : tracks) {
    mQCTracks.processTrack(track);
  }
}

void Tracks::endOfCycle()
{
  mQCTracks.processEndOfCycle();
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void Tracks::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void Tracks::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mQCTracks.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
