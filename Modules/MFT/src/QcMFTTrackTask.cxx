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
/// \author David Grund
/// \author Jakub Juracka
///

// C++
#include <string>
#include <gsl/span>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TString.h>
// O2
#include <DataFormatsMFT/TrackMFT.h>
#include <MFTTracking/TrackCA.h>
#include <Framework/InputRecord.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <DataFormatsITSMFT/Cluster.h>
#include <DataFormatsITSMFT/CompCluster.h>
#include <CommonConstants/LHCConstants.h>
#include <Framework/ProcessingContext.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTTrackTask.h"
#include "MFT/QcMFTUtilTables.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "DetectorsBase/GRPGeomHelper.h"
#include "QualityControl/CustomParameters.h"
#include "QualityControl/ObjectsManager.h"

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

  mNumberOfTracksPerTF = std::make_unique<TH1FRatio>("mMFTTracksPerTF",
                                                     "Number of tracks per TimeFrame; Number of tracks per TF; # entries per orbit", maxTracksPerTF, -0.5, maxTracksPerTF - 0.5, true);
  getObjectsManager()->startPublishing(mNumberOfTracksPerTF.get());
  getObjectsManager()->setDisplayHint(mNumberOfTracksPerTF.get(), "hist");

  mTrackNumberOfClusters = std::make_unique<TH1FRatio>("mMFTTrackNumberOfClusters",
                                                       "Number Of Clusters Per Track; # clusters; # entries per orbit", 10, 0.5, 10.5, true);
  getObjectsManager()->startPublishing(mTrackNumberOfClusters.get());
  getObjectsManager()->setDisplayHint(mTrackNumberOfClusters.get(), "hist");

  mCATrackNumberOfClusters = std::make_unique<TH1FRatio>("CA/mMFTCATrackNumberOfClusters",
                                                         "Number Of Clusters Per CA Track; # clusters; # entries per orbit", 10, 0.5, 10.5, true);
  getObjectsManager()->startPublishing(mCATrackNumberOfClusters.get());
  getObjectsManager()->setDisplayHint(mCATrackNumberOfClusters.get(), "hist");

  mLTFTrackNumberOfClusters = std::make_unique<TH1FRatio>("LTF/mMFTLTFTrackNumberOfClusters",
                                                          "Number Of Clusters Per LTF Track; # clusters; # entries per orbit", 10, 0.5, 10.5, true);
  getObjectsManager()->startPublishing(mLTFTrackNumberOfClusters.get());
  getObjectsManager()->setDisplayHint(mLTFTrackNumberOfClusters.get(), "hist");

  mTrackInvQPt = std::make_unique<TH1FRatio>("mMFTTrackInvQPt", "Track q/p_{T}; q/p_{T} [1/GeV]; # entries per orbit", 250, -10, 10, true);
  getObjectsManager()->startPublishing(mTrackInvQPt.get());
  getObjectsManager()->setDisplayHint(mTrackInvQPt.get(), "hist");

  mTrackChi2 = std::make_unique<TH1FRatio>("mMFTTrackChi2", "Track #chi^{2}/NDF; #chi^{2}/NDF; # entries per orbit", 210, -0.5, 20.5, true);
  getObjectsManager()->startPublishing(mTrackChi2.get());
  getObjectsManager()->setDisplayHint(mTrackChi2.get(), "hist");

  mTrackCharge = std::make_unique<TH1FRatio>("mMFTTrackCharge", "Track Charge; q; # entries per orbit", 3, -1.5, 1.5, true);
  getObjectsManager()->startPublishing(mTrackCharge.get());
  getObjectsManager()->setDisplayHint(mTrackCharge.get(), "hist");

  mTrackPhi = std::make_unique<TH1FRatio>("mMFTTrackPhi", "Track #phi; #phi; # entries per orbit", 100, -3.2, 3.2, true);
  getObjectsManager()->startPublishing(mTrackPhi.get());
  getObjectsManager()->setDisplayHint(mTrackPhi.get(), "hist");

  mPositiveTrackPhi = std::make_unique<TH1FRatio>("mMFTPositiveTrackPhi", "Positive Track #phi; #phi; # entries per orbit", 100, -3.2, 3.2, true);
  getObjectsManager()->startPublishing(mPositiveTrackPhi.get());
  getObjectsManager()->setDisplayHint(mPositiveTrackPhi.get(), "hist");

  mNegativeTrackPhi = std::make_unique<TH1FRatio>("mMFTNegativeTrackPhi", "Negative Track #phi; #phi; # entries per orbit", 100, -3.2, 3.2, true);
  getObjectsManager()->startPublishing(mNegativeTrackPhi.get());
  getObjectsManager()->setDisplayHint(mNegativeTrackPhi.get(), "hist");

  mTrackEta = std::make_unique<TH1FRatio>("mMFTTrackEta", "Track #eta; #eta; # entries per orbit", 50, -4, -2, true);
  getObjectsManager()->startPublishing(mTrackEta.get());
  getObjectsManager()->setDisplayHint(mTrackEta.get(), "hist");

  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto] = std::make_unique<TH1FRatio>(Form("mMFTTrackEta_%d_MinClusters", minNClusters), Form("Track #eta (NCls >= %d); #eta; # entries per orbit", minNClusters), 50, -4, -2, true);
    getObjectsManager()->startPublishing(mTrackEtaNCls[nHisto].get());
    getObjectsManager()->setDisplayHint(mTrackEtaNCls[nHisto].get(), "hist");

    mTrackPhiNCls[nHisto] = std::make_unique<TH1FRatio>(Form("mMFTTrackPhi_%d_MinClusters", minNClusters), Form("Track #phi (NCls >= %d); #phi; # entries per orbit", minNClusters), 100, -3.2, 3.2, true);
    getObjectsManager()->startPublishing(mTrackPhiNCls[nHisto].get());
    getObjectsManager()->setDisplayHint(mTrackPhiNCls[nHisto].get(), "hist");

    mTrackXYNCls[nHisto] = std::make_unique<TH2FRatio>(Form("mMFTTrackXY_%d_MinClusters", minNClusters), Form("Track Position (NCls >= %d); x; y", minNClusters), 320, -16, 16, 320, -16, 16, true);
    getObjectsManager()->startPublishing(mTrackXYNCls[nHisto].get());
    getObjectsManager()->setDisplayHint(mTrackXYNCls[nHisto].get(), "logz colz");

    mTrackEtaPhiNCls[nHisto] = std::make_unique<TH2FRatio>(Form("mMFTTrackEtaPhi_%d_MinClusters", minNClusters), Form("Track #eta , #phi (NCls >= %d); #eta; #phi", minNClusters), 50, -4, -2, 100, -3.2, 3.2, true);
    getObjectsManager()->startPublishing(mTrackEtaPhiNCls[nHisto].get());
    getObjectsManager()->setDisplayHint(mTrackEtaPhiNCls[nHisto].get(), "colz");
  }

  mCATrackEta = std::make_unique<TH1FRatio>("CA/mMFTCATrackEta", "CA Track #eta; #eta; # entries per orbit", 50, -4, -2, true);
  getObjectsManager()->startPublishing(mCATrackEta.get());
  getObjectsManager()->setDisplayHint(mCATrackEta.get(), "hist");

  mLTFTrackEta = std::make_unique<TH1FRatio>("LTF/mMFTLTFTrackEta", "LTF Track #eta; #eta; # entries per orbit", 50, -4, -2, true);
  getObjectsManager()->startPublishing(mLTFTrackEta.get());
  getObjectsManager()->setDisplayHint(mLTFTrackEta.get(), "hist");

  mCATrackPt = std::make_unique<TH1FRatio>("CA/mMFTCATrackPt", "CA Track p_{T}; p_{T} (GeV/c); # entries per orbit", 300, 0, 30, true);
  getObjectsManager()->startPublishing(mCATrackPt.get());
  getObjectsManager()->setDisplayHint(mCATrackPt.get(), "hist logy");

  mLTFTrackPt = std::make_unique<TH1FRatio>("LTF/mMFTLTFTrackPt", "LTF Track p_{T}; p_{T} (GeV/c); # entries per orbit", 300, 0, 30, true);
  getObjectsManager()->startPublishing(mLTFTrackPt.get());
  getObjectsManager()->setDisplayHint(mLTFTrackPt.get(), "hist logy");

  mTrackTanl = std::make_unique<TH1FRatio>("mMFTTrackTanl", "Track tan #lambda; tan #lambda; # entries per orbit", 100, -25, 0, true);
  getObjectsManager()->startPublishing(mTrackTanl.get());
  getObjectsManager()->setDisplayHint(mTrackTanl.get(), "hist");

  mTrackROFNEntries = std::make_unique<TH1FRatio>("mMFTTrackROFSize", "Distribution of the #tracks per ROF; # tracks per ROF; # entries per orbit", QcMFTUtilTables::nROFBins, const_cast<float*>(QcMFTUtilTables::mROFBins), true);
  getObjectsManager()->startPublishing(mTrackROFNEntries.get());
  getObjectsManager()->setDisplayHint(mTrackROFNEntries.get(), "hist logx logy");

  mTracksBC = std::make_unique<TH1FRatio>("mMFTTracksBC", "Tracks per BC; BCid; # entries per orbit", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches, true);
  mTracksBC->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mTracksBC.get());
  getObjectsManager()->setDisplayHint(mTracksBC.get(), "hist");

  mAssociatedClusterFraction = std::make_unique<TH1FRatio>("mAssociatedClusterFraction", "Fraction of clusters in tracks; Clusters in tracks / All clusters; # entries per orbit", 100, 0, 1, true);
  getObjectsManager()->startPublishing(mAssociatedClusterFraction.get());
  getObjectsManager()->setDisplayHint(mAssociatedClusterFraction.get(), "hist logy");

  mClusterRatioVsBunchCrossing = std::make_unique<TH2FRatio>("mClusterRatioVsBunchCrossing", "Bunch Crossing ID vs Cluster Ratio;BC ID;Fraction of clusters in tracks",
                                                             o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches, 100, 0, 1, true);
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
  if (!mGeom) {
    o2::mft::GeometryTGeo::adopt(TaskInterface::retrieveConditionAny<o2::mft::GeometryTGeo>("MFT/Config/Geometry"));
    mGeom = o2::mft::GeometryTGeo::Instance();
    ILOG(Info, Support) << "GeometryTGeo loaded" << ENDM;
  }
  auto mNOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();

  // get the tracks
  const auto tracks = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("tracks");
  if (tracks.empty()) {
    return;
  }
  const auto tracksrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("tracksrofs");
  if (tracksrofs.empty()) {
    return;
  }
  const auto clustersrofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrofs");
  if (clustersrofs.empty()) {
    return;
  }

  // fill the tracks histogram

  mNumberOfTracksPerTF->getNum()->Fill(tracks.size());

  for (int iROF = 0; iROF < tracksrofs.size(); iROF++) {
    int nClusterCountTrack = 0;
    int nTracks = tracksrofs[iROF].getNEntries();

    mTrackROFNEntries->getNum()->Fill(nTracks);

    int start = tracksrofs[iROF].getFirstEntry();
    int end = start + tracksrofs[iROF].getNEntries();

    for (int itrack = start; itrack < end; itrack++) {
      auto& track = tracks[itrack];
      nClusterCountTrack += track.getNumberOfPoints();
    }

    int nTotalClusters = clustersrofs[iROF].getNEntries();

    float clusterRatio = nTotalClusters > 0 ? (float)nClusterCountTrack / (float)nTotalClusters : -1;

    const auto bcData = tracksrofs[iROF].getBCData();
    mTracksBC->getNum()->Fill(bcData.bc, nTracks);
    mAssociatedClusterFraction->getNum()->Fill(clusterRatio);
    mClusterRatioVsBunchCrossing->getNum()->Fill(bcData.bc, clusterRatio);
  }

  for (auto& oneTrack : tracks) {
    mTrackNumberOfClusters->getNum()->Fill(oneTrack.getNumberOfPoints());
    mTrackChi2->getNum()->Fill(oneTrack.getChi2OverNDF());
    mTrackCharge->getNum()->Fill(oneTrack.getCharge());
    mTrackPhi->getNum()->Fill(oneTrack.getPhi());
    mTrackEta->getNum()->Fill(oneTrack.getEta());
    mTrackTanl->getNum()->Fill(oneTrack.getTanl());

    for (auto minNClusters : sMinNClustersList) {
      if (oneTrack.getNumberOfPoints() >= minNClusters) {
        mTrackEtaNCls[minNClusters - sMinNClustersList[0]]->getNum()->Fill(oneTrack.getEta());
        mTrackPhiNCls[minNClusters - sMinNClustersList[0]]->getNum()->Fill(oneTrack.getPhi());
        mTrackXYNCls[minNClusters - sMinNClustersList[0]]->getNum()->Fill(oneTrack.getX(), oneTrack.getY());
        mTrackEtaPhiNCls[minNClusters - sMinNClustersList[0]]->getNum()->Fill(oneTrack.getEta(), oneTrack.getPhi());
      }
    }

    if (oneTrack.getCharge() == +1) {
      mPositiveTrackPhi->getNum()->Fill(oneTrack.getPhi());
      mTrackInvQPt->getNum()->Fill(1 / oneTrack.getPt());
    }

    if (oneTrack.getCharge() == -1) {
      mNegativeTrackPhi->getNum()->Fill(oneTrack.getPhi());
      mTrackInvQPt->getNum()->Fill(-1 / oneTrack.getPt());
    }

    if (oneTrack.isCA()) {
      mCATrackNumberOfClusters->getNum()->Fill(oneTrack.getNumberOfPoints());
      mCATrackEta->getNum()->Fill(oneTrack.getEta());
      mCATrackPt->getNum()->Fill(oneTrack.getPt());
    }
    if (oneTrack.isLTF()) {
      mLTFTrackNumberOfClusters->getNum()->Fill(oneTrack.getNumberOfPoints());
      mLTFTrackEta->getNum()->Fill(oneTrack.getEta());
      mLTFTrackPt->getNum()->Fill(oneTrack.getPt());
    }
  }

  // fill the denominators
  mNumberOfTracksPerTF->getDen()->SetBinContent(1, mNumberOfTracksPerTF->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackNumberOfClusters->getDen()->SetBinContent(1, mTrackNumberOfClusters->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mCATrackNumberOfClusters->getDen()->SetBinContent(1, mCATrackNumberOfClusters->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mLTFTrackNumberOfClusters->getDen()->SetBinContent(1, mLTFTrackNumberOfClusters->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackInvQPt->getDen()->SetBinContent(1, mTrackInvQPt->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackChi2->getDen()->SetBinContent(1, mTrackChi2->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackCharge->getDen()->SetBinContent(1, mTrackCharge->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackPhi->getDen()->SetBinContent(1, mTrackPhi->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mPositiveTrackPhi->getDen()->SetBinContent(1, mPositiveTrackPhi->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mNegativeTrackPhi->getDen()->SetBinContent(1, mNegativeTrackPhi->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackEta->getDen()->SetBinContent(1, mTrackEta->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto]->getDen()->SetBinContent(1, mTrackEtaNCls[nHisto]->getDen()->GetBinContent(1) + mNOrbitsPerTF);
    mTrackPhiNCls[nHisto]->getDen()->SetBinContent(1, mTrackPhiNCls[nHisto]->getDen()->GetBinContent(1) + mNOrbitsPerTF);
    mTrackXYNCls[nHisto]->getDen()->SetBinContent(1, 1, mTrackXYNCls[nHisto]->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
    mTrackEtaPhiNCls[nHisto]->getDen()->SetBinContent(1, 1, mTrackEtaPhiNCls[nHisto]->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
  }
  mCATrackEta->getDen()->SetBinContent(1, mCATrackEta->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mLTFTrackEta->getDen()->SetBinContent(1, mLTFTrackEta->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mCATrackPt->getDen()->SetBinContent(1, mCATrackPt->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mLTFTrackPt->getDen()->SetBinContent(1, mLTFTrackPt->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackTanl->getDen()->SetBinContent(1, mTrackTanl->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTrackROFNEntries->getDen()->SetBinContent(1, mTrackROFNEntries->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mTracksBC->getDen()->SetBinContent(1, mTracksBC->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mAssociatedClusterFraction->getDen()->SetBinContent(1, mAssociatedClusterFraction->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClusterRatioVsBunchCrossing->getDen()->SetBinContent(1, 1, mClusterRatioVsBunchCrossing->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
}

void QcMFTTrackTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  mNumberOfTracksPerTF->update();
  mTrackNumberOfClusters->update();
  mCATrackNumberOfClusters->update();
  mLTFTrackNumberOfClusters->update();
  mTrackInvQPt->update();
  mTrackChi2->update();
  mTrackCharge->update();
  mTrackPhi->update();
  mPositiveTrackPhi->update();
  mNegativeTrackPhi->update();
  mTrackEta->update();
  for (auto minNClusters : sMinNClustersList) {
    auto nHisto = minNClusters - sMinNClustersList[0];
    mTrackEtaNCls[nHisto]->update();
    mTrackPhiNCls[nHisto]->update();
    mTrackXYNCls[nHisto]->update();
    mTrackEtaPhiNCls[nHisto]->update();
  }
  mCATrackEta->update();
  mLTFTrackEta->update();
  mCATrackPt->update();
  mLTFTrackPt->update();
  mTrackTanl->update();
  mTrackROFNEntries->update();
  mTracksBC->update();
  mAssociatedClusterFraction->update();
  mClusterRatioVsBunchCrossing->update();
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
