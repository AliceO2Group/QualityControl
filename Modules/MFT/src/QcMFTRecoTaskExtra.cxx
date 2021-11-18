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
/// \file   QcMFTRecoTaskExtra.cxx
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
#include "MFT/QcMFTRecoTaskExtra.h"

namespace o2::quality_control_modules::mft
{

QcMFTRecoTaskExt::~QcMFTRecoTaskExt()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTRecoTaskExt::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize QcMFTRecoTaskExt" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

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

  auto nHisto = 0;
  for (auto minNClusters : minNClustersList) {
    mTrackEtaNCls[nHisto] = std::make_unique<TH1F>(Form("mMFTTrackEta_%d_MinClusters", minNClusters), Form("Track #eta (NCls >= %d); #eta; # entries", minNClusters), 50, -4, -2);
    getObjectsManager()->startPublishing(mTrackEtaNCls[nHisto].get());
    getObjectsManager()->addMetadata(mTrackEtaNCls[nHisto]->GetName(), "custom", "34");

    mTrackPhiNCls[nHisto] = std::make_unique<TH1F>(Form("mMFTTrackPhi_%d_MinClusters", minNClusters), Form("Track #phi (NCls >= %d); #phi; # entries", minNClusters), 100, -3.2, 3.2);
    getObjectsManager()->startPublishing(mTrackPhiNCls[nHisto].get());
    getObjectsManager()->addMetadata(mTrackPhiNCls[nHisto]->GetName(), "custom", "34");

    mTrackXYNCls[nHisto] = std::make_unique<TH2F>(Form("mMFTTrackXY_%d_MinClusters", minNClusters), Form("Track Position (NCls >= %d); x; y", minNClusters), 320, -16, 16, 320, -16, 16);
    mTrackXYNCls[nHisto]->SetOption("COLZ");
    getObjectsManager()->startPublishing(mTrackXYNCls[nHisto].get());
    getObjectsManager()->addMetadata(mTrackXYNCls[nHisto]->GetName(), "custom", "34");

    nHisto++;
  }

  mCATrackEta = std::make_unique<TH1F>("CA/mMFTCATrackEta", "CA Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mCATrackEta.get());
  getObjectsManager()->addMetadata(mCATrackEta->GetName(), "custom", "34");

  mLTFTrackEta = std::make_unique<TH1F>("LTF/mMFTLTFTrackEta", "LTF Track #eta; #eta; # entries", 50, -4, -2);
  getObjectsManager()->startPublishing(mLTFTrackEta.get());
  getObjectsManager()->addMetadata(mLTFTrackEta->GetName(), "custom", "34");

  mTrackTanl = std::make_unique<TH1F>("mMFTTrackTanl", "Track tan #lambda; tan #lambda; # entries", 100, -25, 0);
  getObjectsManager()->startPublishing(mTrackTanl.get());
  getObjectsManager()->addMetadata(mTrackTanl->GetName(), "custom", "34");

  auto MaxClusterROFSize = 5000;
  if (auto param = mCustomParameters.find("MaxClusterROFSize"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MaxClusterROFSize: " << param->second << ENDM;
    MaxClusterROFSize = stoi(param->second);
  }
  mClusterROFNEntries = std::make_unique<TH1F>("mMFTClustersROFSize", "MFT Cluster ROFs size; ROF Size; # entries", MaxClusterROFSize, 0, MaxClusterROFSize);
  getObjectsManager()->startPublishing(mClusterROFNEntries.get());
  getObjectsManager()->addMetadata(mClusterROFNEntries->GetName(), "custom", "34");

  auto MaxTrackROFSize = 1000;
  if (auto param = mCustomParameters.find("MaxTrackROFSize"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MaxTrackROFSize: " << param->second << ENDM;
    MaxTrackROFSize = stoi(param->second);
  }
  mTrackROFNEntries = std::make_unique<TH1F>("mMFTTrackROFSize", "MFT Track ROFs size; ROF Size; # entries", MaxTrackROFSize, 0, MaxTrackROFSize);
  getObjectsManager()->startPublishing(mTrackROFNEntries.get());
  getObjectsManager()->addMetadata(mTrackROFNEntries->GetName(), "custom", "34");

  auto ROFLengthInBC = 198;
  if (auto param = mCustomParameters.find("ROFLengthInBC"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - ROFLengthInBC: " << param->second << ENDM;
    ROFLengthInBC = stoi(param->second);
  }
  auto ROFsPerOrbit = o2::constants::lhc::LHCMaxBunches / ROFLengthInBC;

  mTracksBC = std::make_unique<TH1F>("mtracksBC", "Tracks per BC (sum over orbits); BCid; # entries", ROFsPerOrbit, 0, o2::constants::lhc::LHCMaxBunches);
  mTracksBC->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mTracksBC.get());
  getObjectsManager()->addMetadata(mTracksBC->GetName(), "custom", "34");

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

  mNOfTracksTime = std::make_unique<TH1F>("mNOfTracksTime", "Number of tracks per time bin; time (s); # entries", NofTimeBins, 0, MaxDuration);
  mNOfTracksTime->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mNOfTracksTime.get());
  getObjectsManager()->addMetadata(mNOfTracksTime->GetName(), "custom", "34");

  mNOfClustersTime = std::make_unique<TH1F>("mNOfClustersTime", "Number of clusters per time bin; time (s); # entries", NofTimeBins, 0, MaxDuration);
  mNOfClustersTime->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mNOfClustersTime.get());
  getObjectsManager()->addMetadata(mNOfClustersTime->GetName(), "custom", "34");

  mClusterSensorIndex = std::make_unique<TH1F>("mMFTClusterSensorIndex", "Chip Cluster Occupancy;Chip ID;#Entries", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mClusterSensorIndex.get());

  mClusterPatternIndex = std::make_unique<TH1F>("mMFTClusterPatternIndex", "Cluster Pattern ID;Pattern ID;#Entries", 300, -0.5, 299.5);
  getObjectsManager()->startPublishing(mClusterPatternIndex.get());

  if (auto param = mCustomParameters.find("RefOrbit"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - RefOrbit: " << param->second << ENDM;
    mRefOrbit = static_cast<uint32_t>(stoi(param->second));
  }
}

void QcMFTRecoTaskExt::startOfActivity(Activity& /*activity*/)
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
  auto nHisto = 0;
  for (auto minNClusters : minNClustersList) {
    mTrackEtaNCls[nHisto]->Reset();
    mTrackPhiNCls[nHisto]->Reset();
    mTrackXYNCls[nHisto]->Reset();
  }
  mCATrackEta->Reset();
  mLTFTrackEta->Reset();
  mTrackTanl->Reset();

  mTrackROFNEntries->Reset();
  mClusterROFNEntries->Reset();

  mClusterSensorIndex->Reset();
  mClusterPatternIndex->Reset();
}

void QcMFTRecoTaskExt::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void QcMFTRecoTaskExt::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("tracks");
  const auto tracksrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("tracksrofs");

  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("clusters");
  const auto clustersrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrofs");

  if (!tracks.size()) {
    return;
  }
  // fill the histograms

  for (const auto& rof : tracksrofs) {
    mTrackROFNEntries->Fill(rof.getNEntries());
    mTracksBC->Fill(rof.getBCData().bc, rof.getNEntries());
    float seconds = orbitToSeconds(rof.getBCData().orbit, mRefOrbit) + rof.getBCData().bc * o2::constants::lhc::LHCBunchSpacingNS * 1e-9;
    mNOfTracksTime->Fill(seconds, rof.getNEntries());
  }

  for (const auto& rof : clustersrofs) {
    mClusterROFNEntries->Fill(rof.getNEntries());
    float seconds = orbitToSeconds(rof.getBCData().orbit, mRefOrbit) + rof.getBCData().bc * o2::constants::lhc::LHCBunchSpacingNS * 1e-9;
    mNOfClustersTime->Fill(seconds, rof.getNEntries());
  }

  for (auto& oneCluster : clusters) {
    mClusterSensorIndex->Fill(oneCluster.getSensorID());
    mClusterPatternIndex->Fill(oneCluster.getPatternID());
  }

  for (auto& oneTrack : tracks) {
    mTrackNumberOfClusters->Fill(oneTrack.getNumberOfPoints());
    mTrackChi2->Fill(oneTrack.getTrackChi2());
    mTrackCharge->Fill(oneTrack.getCharge());
    mTrackPhi->Fill(oneTrack.getPhi());
    mTrackEta->Fill(oneTrack.getEta());
    mTrackTanl->Fill(oneTrack.getTanl());

    for (auto minNClusters : minNClustersList) {
      if (oneTrack.getNumberOfPoints() >= minNClusters) {
        mTrackEtaNCls[minNClusters - minNClustersList[0]]->Fill(oneTrack.getEta());
        mTrackPhiNCls[minNClusters - minNClustersList[0]]->Fill(oneTrack.getPhi());
        mTrackXYNCls[minNClusters - minNClustersList[0]]->Fill(oneTrack.getX(), oneTrack.getY());
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

void QcMFTRecoTaskExt::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void QcMFTRecoTaskExt::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void QcMFTRecoTaskExt::reset()
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
  auto nHisto = 0;
  for (auto minNClusters : minNClustersList) {
    mTrackEtaNCls[nHisto]->Reset();
    mTrackPhiNCls[nHisto]->Reset();
    mTrackXYNCls[nHisto]->Reset();
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
