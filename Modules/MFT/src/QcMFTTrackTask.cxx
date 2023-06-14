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
#include <Framework/TimingInfo.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <DataFormatsITSMFT/Cluster.h>
#include <DataFormatsITSMFT/CompCluster.h>
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
  ILOG(Debug, Devel) << "initialize QcMFTTrackTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // Loading custom parameters
  auto MaxTrackROFSize = 1000;
  if (auto param = mCustomParameters.find("MaxTrackROFSize"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - MaxTrackROFSize: " << param->second << ENDM;
    MaxTrackROFSize = stoi(param->second);
  }

  auto ROFLengthInBC = 198;
  if (auto param = mCustomParameters.find("ROFLengthInBC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ROFLengthInBC: " << param->second << ENDM;
    ROFLengthInBC = stoi(param->second);
  }
  auto ROFsPerOrbit = o2::constants::lhc::LHCMaxBunches / ROFLengthInBC;

  auto MaxDuration = 60.f;
  if (auto param = mCustomParameters.find("MaxDuration"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - MaxDuration: " << param->second << ENDM;
    MaxDuration = stof(param->second);
  }

  auto TimeBinSize = 0.01f;
  if (auto param = mCustomParameters.find("TimeBinSize"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - TimeBinSize: " << param->second << ENDM;
    TimeBinSize = stof(param->second);
  }

  auto NofTimeBins = static_cast<int>(MaxDuration / TimeBinSize);

  // Creating histos

  double maxTracksPerTF = 5000;

  mNumberOfTracksPerTF = std::make_unique<TH1F>("mMFTTracksPerTF",
                                                "Number of tracks per TimeFrame; Number of tracks per TF", maxTracksPerTF, -0.5, maxTracksPerTF - 0.5);
  getObjectsManager()->startPublishing(mNumberOfTracksPerTF.get());

  mTrackNumberOfClusters = std::make_unique<TH1F>("mMFTTrackNumberOfClusters",
                                                  "Number Of Clusters Per Track; # clusters; # entries", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mTrackNumberOfClusters.get());

  mCATrackNumberOfClusters = std::make_unique<TH1F>("CA/mMFTCATrackNumberOfClusters",
                                                    "Number Of Clusters Per CA Track; # clusters; # tracks", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mCATrackNumberOfClusters.get());

  mLTFTrackNumberOfClusters = std::make_unique<TH1F>("LTF/mMFTLTFTrackNumberOfClusters",
                                                     "Number Of Clusters Per LTF Track; # clusters; # entries", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mLTFTrackNumberOfClusters.get());

  mTrackInvQPt = std::make_unique<TH1F>("mMFTTrackInvQPt", "Track q/p_{T}; q/p_{T} [1/GeV]; # entries", 250, -10, 10);
  getObjectsManager()->startPublishing(mTrackInvQPt.get());

  mTrackChi2 = std::make_unique<TH1F>("mMFTTrackChi2", "Track #chi^{2}/NDF; #chi^{2}/NDF; # entries", 210, -0.5, 20.5);
  getObjectsManager()->startPublishing(mTrackChi2.get());

  mTrackCharge = std::make_unique<TH1F>("mMFTTrackCharge", "Track Charge; q; # entries", 3, -1.5, 1.5);
  getObjectsManager()->startPublishing(mTrackCharge.get());

  mTrackPhi = std::make_unique<TH1F>("mMFTTrackPhi", "Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mTrackPhi.get());

  mPositiveTrackPhi = std::make_unique<TH1F>("mMFTPositiveTrackPhi", "Positive Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mPositiveTrackPhi.get());

  mNegativeTrackPhi = std::make_unique<TH1F>("mMFTNegativeTrackPhi", "Negative Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mNegativeTrackPhi.get());

  mTrackEta = std::make_unique<TH1F>("mMFTTrackEta", "Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mTrackEta.get());

  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto] = std::make_unique<TH1F>(Form("mMFTTrackEta_%d_MinClusters", minNClusters), Form("Track #eta (NCls >= %d); #eta; # entries", minNClusters), 50, -4, -2);
    getObjectsManager()->startPublishing(mTrackEtaNCls[nHisto].get());

    mTrackPhiNCls[nHisto] = std::make_unique<TH1F>(Form("mMFTTrackPhi_%d_MinClusters", minNClusters), Form("Track #phi (NCls >= %d); #phi; # entries", minNClusters), 100, -3.2, 3.2);
    getObjectsManager()->startPublishing(mTrackPhiNCls[nHisto].get());

    mTrackXYNCls[nHisto] = std::make_unique<TH2F>(Form("mMFTTrackXY_%d_MinClusters", minNClusters), Form("Track Position (NCls >= %d); x; y", minNClusters), 320, -16, 16, 320, -16, 16);
    getObjectsManager()->startPublishing(mTrackXYNCls[nHisto].get());
    getObjectsManager()->setDisplayHint(mTrackXYNCls[nHisto].get(), "logz colz");

    mTrackEtaPhiNCls[nHisto] = std::make_unique<TH2F>(Form("mMFTTrackEtaPhi_%d_MinClusters", minNClusters), Form("Track #eta , #phi (NCls >= %d); #eta; #phi", minNClusters), 50, -4, -2, 100, -3.2, 3.2);
    getObjectsManager()->startPublishing(mTrackEtaPhiNCls[nHisto].get());
    getObjectsManager()->setDisplayHint(mTrackEtaPhiNCls[nHisto].get(), "colz");
  }

  mCATrackEta = std::make_unique<TH1F>("CA/mMFTCATrackEta", "CA Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mCATrackEta.get());

  mLTFTrackEta = std::make_unique<TH1F>("LTF/mMFTLTFTrackEta", "LTF Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mLTFTrackEta.get());

  mCATrackPt = std::make_unique<TH1F>("CA/mMFTCATrackPt", "CA Track p_{T}; p_{T} (GeV/c); # entries", 300, 0, 30);
  getObjectsManager()->startPublishing(mCATrackPt.get());
  getObjectsManager()->setDisplayHint(mCATrackPt.get(), "logy");

  mLTFTrackPt = std::make_unique<TH1F>("LTF/mMFTLTFTrackPt", "LTF Track p_{T}; p_{T} (GeV/c); # entries", 300, 0, 30);
  getObjectsManager()->startPublishing(mLTFTrackPt.get());
  getObjectsManager()->setDisplayHint(mLTFTrackPt.get(), "logy");

  mTrackTanl = std::make_unique<TH1F>("mMFTTrackTanl", "Track tan #lambda; tan #lambda; # entries", 100, -25, 0);
  getObjectsManager()->startPublishing(mTrackTanl.get());

  mTrackROFNEntries = std::make_unique<TH1F>("mMFTTrackROFSize", "Distribution of the #tracks per ROF; # tracks per ROF; # entries", MaxTrackROFSize, 0, MaxTrackROFSize);
  getObjectsManager()->startPublishing(mTrackROFNEntries.get());

  mTracksBC = std::make_unique<TH1F>("mMFTTracksBC", "Tracks per BC; BCid; # entries", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches);
  mTracksBC->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mTracksBC.get());
  getObjectsManager()->setDisplayHint(mTracksBC.get(), "hist");

  mAssociatedClusterFraction = std::make_unique<TH1F>("mAssociatedClusterFraction", "Fraction of clusters in tracks; Clusters in tracks / All clusters; # entries", 100, 0, 1);
  getObjectsManager()->startPublishing(mAssociatedClusterFraction.get());
  getObjectsManager()->setDisplayHint(mAssociatedClusterFraction.get(), "hist logy");

  mClusterRatioVsBunchCrossing = std::make_unique<TH2F>("mClusterRatioVsBunchCrossing", "Bunch Crossing ID vs Cluster Ratio;BC ID;Fraction of clusters in tracks",
                                                        o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches, 100, 0, 1);
  getObjectsManager()->startPublishing(mClusterRatioVsBunchCrossing.get());
  getObjectsManager()->setDisplayHint(mClusterRatioVsBunchCrossing.get(), "colz logz");
}

void QcMFTTrackTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  // reset histograms
  reset();
}

void QcMFTTrackTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void QcMFTTrackTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("tracks");
  const auto tracksrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("tracksrofs");
  const auto clustersrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrofs");

  // fill the tracks histogram

  mNumberOfTracksPerTF->Fill(tracks.size());

  for (int iROF = 0; iROF < tracksrofs.size(); iROF++) {
    int nClusterCountTrack = 0;
    int nTracks = tracksrofs[iROF].getNEntries();

    mTrackROFNEntries->Fill(nTracks);

    int start = tracksrofs[iROF].getFirstEntry();
    int end = start + tracksrofs[iROF].getNEntries();

    for (int itrack = start; itrack < end; itrack++) {
      auto& track = tracks[itrack];
      nClusterCountTrack += track.getNumberOfPoints();
    }

    int nTotalClusters = clustersrofs[iROF].getNEntries();

    float clusterRatio = nTotalClusters > 0 ? (float)nClusterCountTrack / (float)nTotalClusters : -1;

    const auto bcData = tracksrofs[iROF].getBCData();
    mTracksBC->Fill(bcData.bc, nTracks);
    mAssociatedClusterFraction->Fill(clusterRatio);
    mClusterRatioVsBunchCrossing->Fill(bcData.bc, clusterRatio);
  }

  for (auto& oneTrack : tracks) {
    mTrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
    mTrackChi2->Fill(oneTrack.getChi2OverNDF());
    mTrackCharge->Fill(oneTrack.getCharge());
    mTrackPhi->Fill(oneTrack.getPhi());
    mTrackEta->Fill(oneTrack.getEta());
    mTrackTanl->Fill(oneTrack.getTanl());

    for (auto minNClusters : sMinNClustersList) {
      if (oneTrack.getNumberOfPoints() >= minNClusters) {
        mTrackEtaNCls[minNClusters - sMinNClustersList[0]]->Fill(oneTrack.getEta());
        mTrackPhiNCls[minNClusters - sMinNClustersList[0]]->Fill(oneTrack.getPhi());
        mTrackXYNCls[minNClusters - sMinNClustersList[0]]->Fill(oneTrack.getX(), oneTrack.getY());
        mTrackEtaPhiNCls[minNClusters - sMinNClustersList[0]]->Fill(oneTrack.getEta(), oneTrack.getPhi());
      }
    }

    if (oneTrack.getCharge() == +1) {
      mPositiveTrackPhi->Fill(oneTrack.getPhi());
      mTrackInvQPt->Fill(1 / oneTrack.getPt());
    }

    if (oneTrack.getCharge() == -1) {
      mNegativeTrackPhi->Fill(oneTrack.getPhi());
      mTrackInvQPt->Fill(-1 / oneTrack.getPt());
    }

    if (oneTrack.isCA()) {
      mCATrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
      mCATrackEta->Fill(oneTrack.getEta());
      mCATrackPt->Fill(oneTrack.getPt());
    }
    if (oneTrack.isLTF()) {
      mLTFTrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
      mLTFTrackEta->Fill(oneTrack.getEta());
      mLTFTrackPt->Fill(oneTrack.getPt());
    }
  }
}

void QcMFTTrackTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void QcMFTTrackTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void QcMFTTrackTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;

  mNumberOfTracksPerTF->Reset();
  mTrackNumberOfClusters->Reset();
  mCATrackNumberOfClusters->Reset();
  mLTFTrackNumberOfClusters->Reset();
  mTrackInvQPt->Reset();
  mTrackChi2->Reset();
  mTrackCharge->Reset();
  mTrackPhi->Reset();
  mPositiveTrackPhi->Reset();
  mNegativeTrackPhi->Reset();
  mTrackEta->Reset();
  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto]->Reset();
    mTrackPhiNCls[nHisto]->Reset();
    mTrackXYNCls[nHisto]->Reset();
    mTrackEtaPhiNCls[nHisto]->Reset();
  }
  mCATrackEta->Reset();
  mLTFTrackEta->Reset();
  mCATrackPt->Reset();
  mLTFTrackPt->Reset();
  mTrackTanl->Reset();
  mTrackROFNEntries->Reset();
  mTracksBC->Reset();
  mAssociatedClusterFraction->Reset();
  mClusterRatioVsBunchCrossing->Reset();
}

} // namespace o2::quality_control_modules::mft
