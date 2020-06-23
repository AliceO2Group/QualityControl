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
#include <TH2.h>
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

  mMFT_xy_H = std::make_unique<TH2F>("mMFT_xy_H", "mMFT_xy_H", 22, -10.5, 10.5, 22, -10.5, 10.5);
  getObjectsManager()->startPublishing(mMFT_xy_H.get());
  getObjectsManager()->addMetadata(mMFT_xy_H->GetName(), "custom", "34");

  mMFT_pos_phi_H = std::make_unique<TH1F>("mMFT_pos_phi_H", "mMFT_pos_phi_H", 100, -3.145, 3.145);
  getObjectsManager()->startPublishing(mMFT_pos_phi_H.get());
  getObjectsManager()->addMetadata(mMFT_pos_phi_H->GetName(), "custom", "34");

  mMFT_neg_phi_H = std::make_unique<TH1F>("mMFT_neg_phi_H", "mMFT_neg_phi_H", 100, -3.145, 3.145);
  getObjectsManager()->startPublishing(mMFT_neg_phi_H.get());
  getObjectsManager()->addMetadata(mMFT_neg_phi_H->GetName(), "custom", "34");

  mMFT_xy_C = std::make_unique<TCanvas>("mMFT_xy_C", "mMFT_xy_C", 400, 550);
  getObjectsManager()->startPublishing(mMFT_xy_C.get());
  getObjectsManager()->addMetadata(mMFT_xy_C->GetName(), "custom", "34");

}

void BasicTrackQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;

  mMFT_xy_H->Reset();
  mMFT_pos_phi_H->Reset();
  mMFT_neg_phi_H->Reset();

}

void BasicTrackQcTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void BasicTrackQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("randomtrack");
  if (tracks.size() < 1)
    return;
  // fill the histograms
  for (auto& one_track : tracks) {
    mMFT_xy_H->Fill(one_track.getX(), one_track.getY());
    if (one_track.getCharge()==+1) mMFT_pos_phi_H->Fill(one_track.getPhi());
    if (one_track.getCharge()==-1) mMFT_neg_phi_H->Fill(one_track.getPhi());
  }

  // Draw the canvas
  mMFT_xy_C->cd();
  mMFT_xy_H->Draw("text colz");

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
  
  mMFT_xy_H->Reset();
  mMFT_pos_phi_H->Reset();
  mMFT_neg_phi_H->Reset();
}

} // namespace o2::quality_control_modules::mft
