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
/// \file   QcMFTAsyncTask.cxx
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
#include <DataFormatsITSMFT/ROFRecord.h>
#include <DataFormatsITSMFT/Cluster.h>
#include <DataFormatsITSMFT/CompCluster.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTAsyncTask.h"

namespace o2::quality_control_modules::mft
{

QcMFTAsyncTask::~QcMFTAsyncTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTAsyncTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize QcMFTAsyncTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // Loading custom parameters
  auto MaxClusterROFSize = 5000;
  if (auto param = mCustomParameters.find("MaxClusterROFSize"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MaxClusterROFSize: " << param->second << ENDM;
    MaxClusterROFSize = stoi(param->second);
  }

  auto MaxTrackROFSize = 1000;
  if (auto param = mCustomParameters.find("MaxTrackROFSize"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MaxTrackROFSize: " << param->second << ENDM;
    MaxTrackROFSize = stoi(param->second);
  }

  auto ROFLengthInBC = 198;
  if (auto param = mCustomParameters.find("ROFLengthInBC"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - ROFLengthInBC: " << param->second << ENDM;
    ROFLengthInBC = stoi(param->second);
  }
  auto ROFsPerOrbit = o2::constants::lhc::LHCMaxBunches / ROFLengthInBC;

  auto MaxDuration = 60.f;
  if (auto param = mCustomParameters.find("MaxDuration"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MaxDuration: " << param->second << ENDM;
    MaxDuration = stof(param->second);
  }

  auto TimeBinSize = 0.01f;
  if (auto param = mCustomParameters.find("TimeBinSize"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - TimeBinSize: " << param->second << ENDM;
    TimeBinSize = stof(param->second);
  }

  auto NofTimeBins = static_cast<int>(MaxDuration / TimeBinSize);

  if (auto param = mCustomParameters.find("RefOrbit"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - RefOrbit: " << param->second << ENDM;
    mRefOrbit = static_cast<uint32_t>(stoi(param->second));
  }

  // Creating histos
  mTrackNumberOfClusters = std::make_unique<TH1F>("tracks/mMFTTrackNumberOfClusters",
                                                  "Number Of Clusters Per Track; # clusters; # entries", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mTrackNumberOfClusters.get());

  mCATrackNumberOfClusters = std::make_unique<TH1F>("tracks/CA/mMFTCATrackNumberOfClusters",
                                                    "Number Of Clusters Per CA Track; # clusters; # tracks", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mCATrackNumberOfClusters.get());

  mLTFTrackNumberOfClusters = std::make_unique<TH1F>("tracks/LTF/mMFTLTFTrackNumberOfClusters",
                                                     "Number Of Clusters Per LTF Track; # clusters; # entries", 10, 0.5, 10.5);
  getObjectsManager()->startPublishing(mLTFTrackNumberOfClusters.get());

  mTrackOnvQPt = std::make_unique<TH1F>("tracks/mMFTTrackOnvQPt", "Track q/p_{T}; q/p_{T} [1/GeV]; # entries", 50, -2, 2);
  getObjectsManager()->startPublishing(mTrackOnvQPt.get());

  mTrackChi2 = std::make_unique<TH1F>("tracks/mMFTTrackChi2", "Track #chi^{2}; #chi^{2}; # entries", 21, -0.5, 20.5);
  getObjectsManager()->startPublishing(mTrackChi2.get());

  mTrackCharge = std::make_unique<TH1F>("tracks/mMFTTrackCharge", "Track Charge; q; # entries", 3, -1.5, 1.5);
  getObjectsManager()->startPublishing(mTrackCharge.get());

  mTrackPhi = std::make_unique<TH1F>("tracks/mMFTTrackPhi", "Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mTrackPhi.get());

  mPositiveTrackPhi = std::make_unique<TH1F>("tracks/mMFTPositiveTrackPhi", "Positive Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mPositiveTrackPhi.get());

  mNegativeTrackPhi = std::make_unique<TH1F>("tracks/mMFTNegativeTrackPhi", "Negative Track #phi; #phi; # entries", 100, -3.2, 3.2);
  getObjectsManager()->startPublishing(mNegativeTrackPhi.get());

  mTrackEta = std::make_unique<TH1F>("tracks/mMFTTrackEta", "Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mTrackEta.get());

  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto] = std::make_unique<TH1F>(Form("tracks/mMFTTrackEta_%d_MinClusters", minNClusters), Form("Track #eta (NCls >= %d); #eta; # entries", minNClusters), 50, -4, -2);
    getObjectsManager()->startPublishing(mTrackEtaNCls[nHisto].get());

    mTrackPhiNCls[nHisto] = std::make_unique<TH1F>(Form("tracks/mMFTTrackPhi_%d_MinClusters", minNClusters), Form("Track #phi (NCls >= %d); #phi; # entries", minNClusters), 100, -3.2, 3.2);
    getObjectsManager()->startPublishing(mTrackPhiNCls[nHisto].get());

    mTrackXYNCls[nHisto] = std::make_unique<TH2F>(Form("tracks/mMFTTrackXY_%d_MinClusters", minNClusters), Form("Track Position (NCls >= %d); x; y", minNClusters), 320, -16, 16, 320, -16, 16);
    mTrackXYNCls[nHisto]->SetOption("COLZ");
    getObjectsManager()->startPublishing(mTrackXYNCls[nHisto].get());

    mTrackEtaPhiNCls[nHisto] = std::make_unique<TH2F>(Form("tracks/mMFTTrackEtaPhi_%d_MinClusters", minNClusters), Form("Track #eta , #phi (NCls >= %d); #eta; #phi", minNClusters), 50, -4, -2, 100, -3.2, 3.2);
    mTrackEtaPhiNCls[nHisto]->SetOption("COLZ");
    getObjectsManager()->startPublishing(mTrackEtaPhiNCls[nHisto].get());
  }

  mCATrackEta = std::make_unique<TH1F>("tracks/CA/mMFTCATrackEta", "CA Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mCATrackEta.get());

  mLTFTrackEta = std::make_unique<TH1F>("tracks/LTF/mMFTLTFTrackEta", "LTF Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mLTFTrackEta.get());

  mTrackTanl = std::make_unique<TH1F>("tracks/mMFTTrackTanl", "Track tan #lambda; tan #lambda; # entries", 100, -25, 0);
  getObjectsManager()->startPublishing(mTrackTanl.get());

  mClusterROFNEntries = std::make_unique<TH1F>("clusters/mMFTClustersROFSize", "MFT Cluster ROFs size; ROF Size; # entries", MaxClusterROFSize, 0, MaxClusterROFSize);
  getObjectsManager()->startPublishing(mClusterROFNEntries.get());

  mTrackROFNEntries = std::make_unique<TH1F>("tracks/mMFTTrackROFSize", "MFT Track ROFs size; ROF Size; # entries", MaxTrackROFSize, 0, MaxTrackROFSize);
  getObjectsManager()->startPublishing(mTrackROFNEntries.get());

  mTracksBC = std::make_unique<TH1F>("tracks/mMFTTracksBC", "Tracks per BC (sum over orbits); BCid; # entries", ROFsPerOrbit, 0, o2::constants::lhc::LHCMaxBunches);
  mTracksBC->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mTracksBC.get());

  mNOfTracksTime = std::make_unique<TH1F>("tracks/mNOfTracksTime", "Number of tracks per time bin; time (s); # entries", NofTimeBins, 0, MaxDuration);
  mNOfTracksTime->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mNOfTracksTime.get());

  mNOfClustersTime = std::make_unique<TH1F>("clusters/mNOfClustersTime", "Number of clusters per time bin; time (s); # entries", NofTimeBins, 0, MaxDuration);
  mNOfClustersTime->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mNOfClustersTime.get());

  mClusterSensorIndex = std::make_unique<TH1F>("clusters/mMFTClusterSensorIndex", "Chip Cluster Occupancy;Chip ID;#Entries", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mClusterSensorIndex.get());

  mClusterPatternIndex = std::make_unique<TH1F>("clusters/mMFTClusterPatternIndex", "Cluster Pattern ID;Pattern ID;#Entries", 300, -0.5, 299.5);
  getObjectsManager()->startPublishing(mClusterPatternIndex.get());
}

void QcMFTAsyncTask::startOfActivity(Activity& /*activity*/)
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
  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto]->Reset();
    mTrackPhiNCls[nHisto]->Reset();
    mTrackXYNCls[nHisto]->Reset();
    mTrackEtaPhiNCls[nHisto]->Reset();
  }
  mCATrackEta->Reset();
  mLTFTrackEta->Reset();
  mTrackTanl->Reset();

  mTrackROFNEntries->Reset();
  mClusterROFNEntries->Reset();

  mClusterSensorIndex->Reset();
  mClusterPatternIndex->Reset();
}

void QcMFTAsyncTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void QcMFTAsyncTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("tracks");
  const auto tracksrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("tracksrofs");

  // get clusters
  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("clusters");
  const auto clustersrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrofs");

  // Fill the clusters histograms
  for (const auto& rof : clustersrofs) {
    mClusterROFNEntries->Fill(rof.getNEntries());
    float seconds = orbitToSeconds(rof.getBCData().orbit, mRefOrbit) + rof.getBCData().bc * o2::constants::lhc::LHCBunchSpacingNS * 1e-9;
    mNOfClustersTime->Fill(seconds, rof.getNEntries());
  }

  for (auto& oneCluster : clusters) {
    mClusterSensorIndex->Fill(oneCluster.getSensorID());
    mClusterPatternIndex->Fill(oneCluster.getPatternID());
  }

  // fill the tracks histogram

  for (const auto& rof : tracksrofs) {
    mTrackROFNEntries->Fill(rof.getNEntries());
    mTracksBC->Fill(rof.getBCData().bc, rof.getNEntries());
    float seconds = orbitToSeconds(rof.getBCData().orbit, mRefOrbit) + rof.getBCData().bc * o2::constants::lhc::LHCBunchSpacingNS * 1e-9;
    mNOfTracksTime->Fill(seconds, rof.getNEntries());
  }

  for (auto& oneTrack : tracks) {
    mTrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
    mTrackChi2->Fill(oneTrack.getTrackChi2());
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

void QcMFTAsyncTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void QcMFTAsyncTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void QcMFTAsyncTask::reset()
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
  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto]->Reset();
    mTrackPhiNCls[nHisto]->Reset();
    mTrackXYNCls[nHisto]->Reset();
    mTrackEtaPhiNCls[nHisto]->Reset();
  }
  mCATrackEta->Reset();
  mLTFTrackEta->Reset();
  mTrackTanl->Reset();

  mTrackROFNEntries->Reset();
  mClusterROFNEntries->Reset();

  mClusterSensorIndex->Reset();
  mClusterPatternIndex->Reset();
}

} // namespace o2::quality_control_modules::mft
