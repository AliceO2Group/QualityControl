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
/// \file   TracksQcTask.cxx
/// \author Valerie Ramillien
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "QualityControl/QcInfoLogger.h"
#include "MID/TracksQcTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "DataFormatsMID/Track.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"

#define MID_NDE 72
#define DZpos 10

namespace o2::quality_control_modules::mid
{

TracksQcTask::~TracksQcTask()
{
}

void TracksQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TracksQcTask" << ENDM;

  mHistogram = std::make_shared<TH1F>("exemple", "exemple", 2000, -2000, 2000);
  getObjectsManager()->startPublishing(mHistogram.get());

  mTrackBCCounts = std::make_shared<TH1F>("TrackBCCounts", "Cluster Bunch Crossing Counts", o2::constants::lhc::LHCMaxBunches, 0., o2::constants::lhc::LHCMaxBunches);
  getObjectsManager()->startPublishing(mTrackBCCounts.get());
  mTrackBCCounts->GetXaxis()->SetTitle("BC");
  mTrackBCCounts->GetYaxis()->SetTitle("Entry");

  mMultTracks = std::make_shared<TH1F>("MultTracks", "Multiplicity Tracks ", 100, 0, 100);
  getObjectsManager()->startPublishing(mMultTracks.get());

  mTrackMapXY = std::make_shared<TH2F>("TrackMapXY", "Track Map X-Y ", 300, -300., 300., 300, -300., 300.);
  getObjectsManager()->startPublishing(mTrackMapXY.get());
  mTrackMapXY->GetXaxis()->SetTitle("X Position (cm)");
  mTrackMapXY->GetYaxis()->SetTitle("Y Position (cm)");
  mTrackMapXY->SetOption("colz");
  mTrackMapXY->SetStats(0);

  mTrackDevXY = std::make_shared<TH2F>("TrackDevXY", "Track Deviation X-Y", 300, -300., 300., 300, -500., 500.);
  getObjectsManager()->startPublishing(mTrackDevXY.get());
  mTrackDevXY->GetXaxis()->SetTitle("X Dev (cm)");
  mTrackDevXY->GetYaxis()->SetTitle("Y Dev (cm)");
  mTrackDevXY->SetOption("colz");
  mTrackDevXY->SetStats(0);

  mTrackDevX = std::make_shared<TH1F>("TrackDevX", "Track Deviation X", 300, -300., 300.);
  getObjectsManager()->startPublishing(mTrackDevX.get());
  mTrackDevX->GetXaxis()->SetTitle("Track Dev X (cm)");
  mTrackDevX->GetYaxis()->SetTitle("Entry");

  mTrackDevY = std::make_shared<TH1F>("TrackDevY", "Track Deviation Y", 300, -300., 300.);
  getObjectsManager()->startPublishing(mTrackDevY.get());
  mTrackDevY->GetXaxis()->SetTitle("Track Dev Y (cm)");
  mTrackDevY->GetYaxis()->SetTitle("Entry");
}
void TracksQcTask::startOfActivity(Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mHistogram->Reset();
}

void TracksQcTask::startOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TracksQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto tracks = ctx.inputs().get<gsl::span<o2::mid::Track>>("tracks");
  auto rofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("trackrofs");

  int multTracks = 0;
  float DeltaZ = o2::mid::geoparams::DefaultChamberZ[4] - o2::mid::geoparams::DefaultChamberZ[1];

  for (const auto& rofRecord : rofs) { // loop ROFRecords == Events //
    // printf("========================================================== \n");
    // printf("%05d ROF with first entry %05zu and nentries %02zu , BC %05d, ORB %05d , EventType %02d\n", nROF, rofRecord.firstEntry, rofRecord.nEntries, rofRecord.interactionRecord.bc, rofRecord.interactionRecord.orbit,rofRecord.eventType);

    multTracks = 0;
    nROF++;
    mTrackBCCounts->Fill(rofRecord.interactionRecord.bc);

    for (auto& track : tracks.subspan(rofRecord.firstEntry, rofRecord.nEntries)) { // loop Tracks in ROF//
      multTracks += 1;
      // std::cout << "  =>>  " << track << std::endl;
      //  Loop on X +/- EX  Y +/- EY ??
      mHistogram->Fill(track.getPositionZ()); /// test
      mTrackMapXY->Fill(track.getPositionX(), track.getPositionY(), 1);
      mTrackDevX->Fill(track.getDirectionX() * DeltaZ);
      mTrackDevY->Fill(track.getDirectionY() * DeltaZ);
      mTrackDevXY->Fill(track.getDirectionX() * DeltaZ, track.getDirectionY() * DeltaZ, 1);

    } // tracks in ROF
    mMultTracks->Fill(multTracks);
  } //  ROFRecords //
}

void TracksQcTask::endOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void TracksQcTask::endOfActivity(Activity& /*activity*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TracksQcTask::reset()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistogram->Reset();
  mTrackMapXY->Reset();
  mTrackDevX->Reset();
  mTrackDevY->Reset();
  mTrackDevXY->Reset();

  mTrackBCCounts->Reset();
}

} // namespace o2::quality_control_modules::mid
