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
/// \author Diana Maria Krupova

// ROOT
#include <TH1.h>
#include <TH2.h>
// O2
#include <DataFormatsMFT/TrackMFT.h>
#include <MFTTracking/TrackCA.h>
#include <Framework/InputRecord.h>
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
  ILOG(Info, Support) << "initialize BasicTrackQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mMFT_number_of_clusters_per_track_H = std::make_unique<TH1F>("mMFT_number_of_clusters_per_track_H", "mMFT_number_of_clusters_per_track_H", 10, -0.5, 9.5);
  getObjectsManager()->startPublishing(mMFT_number_of_clusters_per_track_H.get());
  getObjectsManager()->addMetadata(mMFT_number_of_clusters_per_track_H->GetName(), "custom", "34");

  mMFT_number_of_clusters_per_CA_track_H = std::make_unique<TH1F>("mMFT_number_of_clusters_per_CA_track_H", "mMFT_number_of_clusters_per_CA_track_H", 10, -0.5, 9.5);
  getObjectsManager()->startPublishing(mMFT_number_of_clusters_per_CA_track_H.get());

  mMFT_number_of_clusters_per_LTF_track_H = std::make_unique<TH1F>("mMFT_number_of_clusters_per_LTF_track_H", "mMFT_number_of_clusters_per_LTF_track_H", 10, -0.5, 9.5);
  getObjectsManager()->startPublishing(mMFT_number_of_clusters_per_LTF_track_H.get());

  mMFT_track_inv_qpt_H = std::make_unique<TH1F>("mMFT_track_inv_qpt_H", "mMFT_track_inv_qpt_H", 100, -5, 5);
  getObjectsManager()->startPublishing(mMFT_track_inv_qpt_H.get());

  mMFT_track_chi2_H = std::make_unique<TH1F>("mMFT_track_chi2_H", "mMFT_track_chi2_H", 81, -0.5, 80.5);
  getObjectsManager()->startPublishing(mMFT_track_chi2_H.get());

  mMFT_charge_H = std::make_unique<TH1F>("mMFT_charge_H", "mMFT_charge_H", 3, -1.5, 1.5);
  getObjectsManager()->startPublishing(mMFT_charge_H.get());

  mMFT_phi_H = std::make_unique<TH1F>("mMFT_phi_H", "mMFT_phi_H", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mMFT_phi_H.get());

  mMFT_pos_phi_H = std::make_unique<TH1F>("mMFT_pos_phi_H", "mMFT_pos_phi_H", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mMFT_pos_phi_H.get());

  mMFT_neg_phi_H = std::make_unique<TH1F>("mMFT_neg_phi_H", "mMFT_neg_phi_H", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mMFT_neg_phi_H.get());

  mMFT_eta_track_H = std::make_unique<TH1F>("mMFT_eta_track_H", "mMFT_eta_track_H", 100, -5, -1);
  getObjectsManager()->startPublishing(mMFT_eta_track_H.get());

  mMFT_eta_CA_track_H = std::make_unique<TH1F>("mMFT_eta_CA_track_H", "mMFT_eta_CA_track_H", 100, -5, -1);
  getObjectsManager()->startPublishing(mMFT_eta_CA_track_H.get());

  mMFT_eta_LTF_track_H = std::make_unique<TH1F>("mMFT_eta_LTF_track_H", "mMFT_eta_LTF_track_H", 100, -5, -1);
  getObjectsManager()->startPublishing(mMFT_eta_LTF_track_H.get());

  mMFT_tang_lambda_H = std::make_unique<TH1F>("mMFT_tang_lambda_H", "mMFT_tang_lambda_H", 100, -25, 0);
  getObjectsManager()->startPublishing(mMFT_tang_lambda_H.get());
}

void BasicTrackQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;

  mMFT_number_of_clusters_per_track_H->Reset();
  mMFT_number_of_clusters_per_CA_track_H->Reset();
  mMFT_number_of_clusters_per_LTF_track_H->Reset();
  mMFT_track_inv_qpt_H->Reset();
  mMFT_track_chi2_H->Reset();
  mMFT_charge_H->Reset();
  mMFT_phi_H->Reset();
  mMFT_pos_phi_H->Reset();
  mMFT_neg_phi_H->Reset();
  mMFT_eta_track_H->Reset();
  mMFT_eta_CA_track_H->Reset();
  mMFT_eta_LTF_track_H->Reset();
  mMFT_tang_lambda_H->Reset();
}

void BasicTrackQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void BasicTrackQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("randomtrack");
  if (tracks.size() < 1)
    return;
  // fill the histograms
  for (auto& one_track : tracks) {
    mMFT_number_of_clusters_per_track_H->Fill(one_track.getNumberOfPoints());
    mMFT_track_chi2_H->Fill(one_track.getTrackChi2());
    mMFT_charge_H->Fill(one_track.getCharge());
    mMFT_phi_H->Fill(one_track.getPhi());
    mMFT_eta_track_H->Fill(one_track.getEta());
    mMFT_tang_lambda_H->Fill(one_track.getTanl());

    if (one_track.getCharge() == +1) {
      mMFT_pos_phi_H->Fill(one_track.getPhi());
      mMFT_track_inv_qpt_H->Fill(1 / one_track.getPt());
    }

    if (one_track.getCharge() == -1) {
      mMFT_neg_phi_H->Fill(one_track.getPhi());
      mMFT_track_inv_qpt_H->Fill(-1 / one_track.getPt());
    }

    if (one_track.isCA()) {
      mMFT_number_of_clusters_per_CA_track_H->Fill(one_track.getNumberOfPoints());
      mMFT_eta_CA_track_H->Fill(one_track.getEta());
    }
    if (one_track.isLTF()) {
      mMFT_number_of_clusters_per_LTF_track_H->Fill(one_track.getNumberOfPoints());
      mMFT_eta_LTF_track_H->Fill(one_track.getEta());
    }
  }
}

void BasicTrackQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicTrackQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicTrackQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mMFT_number_of_clusters_per_track_H->Reset();
  mMFT_number_of_clusters_per_CA_track_H->Reset();
  mMFT_number_of_clusters_per_LTF_track_H->Reset();
  mMFT_track_inv_qpt_H->Reset();
  mMFT_track_chi2_H->Reset();
  mMFT_charge_H->Reset();
  mMFT_phi_H->Reset();
  mMFT_pos_phi_H->Reset();
  mMFT_neg_phi_H->Reset();
  mMFT_eta_track_H->Reset();
  mMFT_eta_CA_track_H->Reset();
  mMFT_eta_LTF_track_H->Reset();
  mMFT_tang_lambda_H->Reset();
}

} // namespace o2::quality_control_modules::mft
