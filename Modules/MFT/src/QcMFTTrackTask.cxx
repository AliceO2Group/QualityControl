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
/// \file   QcMFTTrackTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Diana Maria Krupova
/// \author Katarina Krizkova Gajdosova

// ROOT
#include <TH1.h>
#include <TH2.h>
// O2
#include <DataFormatsMFT/TrackMFT.h>
#include <MFTTracking/TrackCA.h>
#include <Framework/InputRecord.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTTrackTask.h"

namespace o2::quality_control_modules::mft
{

QcMFTTrackTask::~QcMFTTrackTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTTrackTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize QcMFTTrackTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mTrackNumberOfClusters = std::make_unique<TH1F>("mMFTTrackNumberOfClusters",
                                                  "Number Of Clusters Per Track; # clusters; # entries", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mTrackNumberOfClusters.get());
  getObjectsManager()->addMetadata(mTrackNumberOfClusters->GetName(), "custom", "34");

  mCATrackNumberOfClusters = std::make_unique<TH1F>("CA/mMFTCATrackNumberOfClusters",
                                                    "Number Of Clusters Per CA Track; # clusters; # tracks", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mCATrackNumberOfClusters.get());
  getObjectsManager()->addMetadata(mCATrackNumberOfClusters->GetName(), "custom", "34");

  mLTFTrackNumberOfClusters = std::make_unique<TH1F>("LTF/mMFTLTFTrackNumberOfClusters",
                                                     "Number Of Clusters Per LTF Track; # clusters; # entries", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mLTFTrackNumberOfClusters.get());
  getObjectsManager()->addMetadata(mLTFTrackNumberOfClusters->GetName(), "custom", "34");

  mTrackOnvQPt = std::make_unique<TH1F>("mMFTTrackOnvQPt", "Track q/p_{T}; q/p_{T} [1/GeV]; # entries", 50, -2, 2);
  getObjectsManager()->startPublishing(mTrackOnvQPt.get());
  getObjectsManager()->addMetadata(mTrackOnvQPt->GetName(), "custom", "34");

  mTrackChi2 = std::make_unique<TH1F>("mMFTTrackChi2", "Track #chi^{2}; #chi^{2}; # entries", 21, -0.5, 20.5);
  getObjectsManager()->startPublishing(mTrackChi2.get());
  getObjectsManager()->addMetadata(mTrackChi2->GetName(), "custom", "34");

  mTrackCharge = std::make_unique<TH1F>("mMFTTrackCharge", "Track Charge; q; # entries", 3, -1.5, 1.5);
  getObjectsManager()->startPublishing(mTrackCharge.get());
  getObjectsManager()->addMetadata(mTrackCharge->GetName(), "custom", "34");

  mTrackPhi = std::make_unique<TH1F>("mMFTTrackPhi", "Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mTrackPhi.get());
  getObjectsManager()->addMetadata(mTrackPhi->GetName(), "custom", "34");

  mPositiveTrackPhi = std::make_unique<TH1F>("mMFTPositiveTrackPhi", "Positive Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mPositiveTrackPhi.get());
  getObjectsManager()->addMetadata(mPositiveTrackPhi->GetName(), "custom", "34");

  mNegativeTrackPhi = std::make_unique<TH1F>("mMFTNegativeTrackPhi", "Negative Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mNegativeTrackPhi.get());
  getObjectsManager()->addMetadata(mNegativeTrackPhi->GetName(), "custom", "34");

  mTrackEta = std::make_unique<TH1F>("mMFTTrackEta", "Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mTrackEta.get());
  getObjectsManager()->addMetadata(mTrackEta->GetName(), "custom", "34");

  mCATrackEta = std::make_unique<TH1F>("CA/mMFTCATrackEta", "CA Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mCATrackEta.get());
  getObjectsManager()->addMetadata(mCATrackEta->GetName(), "custom", "34");

  mLTFTrackEta = std::make_unique<TH1F>("LTF/mMFTLTFTrackEta", "LTF Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mLTFTrackEta.get());
  getObjectsManager()->addMetadata(mLTFTrackEta->GetName(), "custom", "34");

  mTrackTanl = std::make_unique<TH1F>("mMFTTrackTanl", "Track tan #lambda; tan #lambda; # entries", 100, -25, 0);
  getObjectsManager()->startPublishing(mTrackTanl.get());
  getObjectsManager()->addMetadata(mTrackTanl->GetName(), "custom", "34");
}

void QcMFTTrackTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;

  mTrackNumberOfClusters->Reset();
  mCATrackNumberOfClusters->Reset();
  mLTFTrackNumberOfClusters->Reset();
  mTrackOnvQPt->Reset();
  mTrackChi2->Reset();
  mTrackCharge->Reset();
  mTrackPhi->Reset();
  mPositiveTrackPhi->Reset();
  mNegativeTrackPhi->Reset();
  mTrackEta->Reset();
  mCATrackEta->Reset();
  mLTFTrackEta->Reset();
  mTrackTanl->Reset();
}

void QcMFTTrackTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void QcMFTTrackTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("randomtrack");
  if (tracks.size() < 1)
    return;
  // fill the histograms
  for (auto& oneTrack : tracks) {
    mTrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
    mTrackChi2->Fill(oneTrack.getTrackChi2());
    mTrackCharge->Fill(oneTrack.getCharge());
    mTrackPhi->Fill(oneTrack.getPhi());
    mTrackEta->Fill(oneTrack.getEta());
    mTrackTanl->Fill(oneTrack.getTanl());

    if (oneTrack.getCharge() == +1) {
      mPositiveTrackPhi->Fill(oneTrack.getPhi());
      mTrackOnvQPt->Fill(1 / oneTrack.getPt());
    }

    if (oneTrack.getCharge() == -1) {
      mNegativeTrackPhi->Fill(oneTrack.getPhi());
      mTrackOnvQPt->Fill(-1 / oneTrack.getPt());
    }

    if (oneTrack.isCA()) {
      mCATrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
      mCATrackEta->Fill(oneTrack.getEta());
    }
    if (oneTrack.isLTF()) {
      mLTFTrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
      mLTFTrackEta->Fill(oneTrack.getEta());
    }
  }
}

void QcMFTTrackTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void QcMFTTrackTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void QcMFTTrackTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mTrackNumberOfClusters->Reset();
  mCATrackNumberOfClusters->Reset();
  mLTFTrackNumberOfClusters->Reset();
  mTrackOnvQPt->Reset();
  mTrackChi2->Reset();
  mTrackCharge->Reset();
  mTrackPhi->Reset();
  mPositiveTrackPhi->Reset();
  mNegativeTrackPhi->Reset();
  mTrackEta->Reset();
  mCATrackEta->Reset();
  mLTFTrackEta->Reset();
  mTrackTanl->Reset();
}

} // namespace o2::quality_control_modules::mft
