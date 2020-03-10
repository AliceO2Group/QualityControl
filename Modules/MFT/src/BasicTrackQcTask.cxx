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
/// \file   BasicTrackQcTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
///

// ROOT
#include <TCanvas.h>
#include <TH1.h>
// O2
#include <DataFormatsMFT/TrackMFT.h>
#include <MFTTracking/TrackCA.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/BasicTrackQcTask.h"

namespace o2::quality_control_modules::mft
{

BasicTrackQcTask::~BasicTrackQcTask()
{
  /*
    not needed for unique pointers
  */
}

void BasicTrackQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize BasicTrackQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mMFT_nPoints_H = std::make_unique<TH1F>("mMFT_nPoints", "mMFT_nPoints", 12, -0.5, 11.5);
  getObjectsManager()->startPublishing(mMFT_nPoints_H.get());
  getObjectsManager()->addMetadata(mMFT_nPoints_H->GetName(), "custom", "34");
}

void BasicTrackQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  mMFT_nPoints_H->Reset();
}

void BasicTrackQcTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void BasicTrackQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackLTF>>("randomtrack");
  if (tracks.size() < 1)
    return;
  // fill the histograms
  for (auto& one_track : tracks) {
    mMFT_nPoints_H->Fill(one_track.getNPoints());
  }
}

void BasicTrackQcTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void BasicTrackQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void BasicTrackQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mMFT_nPoints_H->Reset();
}

} // namespace o2::quality_control_modules::mft
