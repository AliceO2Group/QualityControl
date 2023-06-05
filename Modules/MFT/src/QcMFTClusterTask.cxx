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
/// \file   QcMFTClusterTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2F.h>

// O2
#include <DataFormatsITSMFT/Cluster.h>
#include <DataFormatsITSMFT/CompCluster.h>
#include <Framework/InputRecord.h>
#include <Framework/TimingInfo.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <DataFormatsITSMFT/ClusterTopology.h>
#include <ITSMFTReconstruction/ChipMappingMFT.h>
#include "CCDB/BasicCCDBManager.h"
#include "CCDB/CCDBTimeStampUtils.h"
#include "MFTTracking/IOUtils.h"
#include "MFTBase/GeometryTGeo.h"
#include <DetectorsBase/GeometryManager.h>

// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTClusterTask.h"
#include "MFT/QcMFTUtilTables.h"

// C++
#include <fstream>

using namespace o2::mft;
o2::itsmft::ChipMappingMFT mMFTMapper;

namespace o2::quality_control_modules::mft
{

QcMFTClusterTask::QcMFTClusterTask()
  : TaskInterface()
{
}

QcMFTClusterTask::~QcMFTClusterTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize QcMFTClusterTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  // loading custom parameters
  auto maxClusterROFSize = 5000;
  if (auto param = mCustomParameters.find("maxClusterROFSize"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - maxClusterROFSize: " << param->second << ENDM;
    maxClusterROFSize = stoi(param->second);
  }

  auto maxDuration = 60.f;
  if (auto param = mCustomParameters.find("maxDuration"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - maxDuration: " << param->second << ENDM;
    maxDuration = stof(param->second);
  }

  auto timeBinSize = 0.01f;
  if (auto param = mCustomParameters.find("timeBinSize"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - timeBinSize: " << param->second << ENDM;
    timeBinSize = stof(param->second);
  }

  auto NofTimeBins = static_cast<int>(maxDuration / timeBinSize);

  auto ROFLengthInBC = 198;
  if (auto param = mCustomParameters.find("ROFLengthInBC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ROFLengthInBC: " << param->second << ENDM;
    ROFLengthInBC = stoi(param->second);
  }
  auto ROFsPerOrbit = o2::constants::lhc::LHCMaxBunches / ROFLengthInBC;

  if (auto param = mCustomParameters.find("geomFileName"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - geometry filename: " << param->second << ENDM;
    mGeomPath = param->second;
  }

  getChipMapData();

  // define histograms
  mClusterLayerIndexH0 = std::make_unique<TH1F>("mClusterLayerIndexH0", "Clusters per layer in H0;Layer;Entries", 10, -0.5, 9.5);
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(1, "d0-f0");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(2, "d0-f1");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(3, "d1-f0");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(4, "d1-f1");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(5, "d2-f0");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(6, "d2-f1");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(7, "d3-f0");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(8, "d3-f1");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(9, "d4-f0");
  mClusterLayerIndexH0->GetXaxis()->SetBinLabel(10, "d4-f1");
  mClusterLayerIndexH0->SetStats(0);
  getObjectsManager()->startPublishing(mClusterLayerIndexH0.get());

  mClusterLayerIndexH1 = std::make_unique<TH1F>("mClusterLayerIndexH1", "Clusters per layer in H1;Layer;Entries", 10, -0.5, 9.5);
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(1, "d0-f0");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(2, "d0-f1");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(3, "d1-f0");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(4, "d1-f1");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(5, "d2-f0");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(6, "d2-f1");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(7, "d3-f0");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(8, "d3-f1");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(9, "d4-f0");
  mClusterLayerIndexH1->GetXaxis()->SetBinLabel(10, "d4-f1");
  mClusterLayerIndexH1->SetStats(0);
  getObjectsManager()->startPublishing(mClusterLayerIndexH1.get());

  mClusterOccupancy = std::make_unique<TH1F>("mClusterOccupancy", "Chip Cluster Occupancy;Chip ID;#Entries per TF", 936, -0.5, 935.5);
  mClusterOccupancy->SetStats(0);
  getObjectsManager()->startPublishing(mClusterOccupancy.get());

  mClusterPatternIndex = std::make_unique<TH1F>("mClusterPatternIndex", "Cluster Pattern ID;Pattern ID;#Entries per TF", 500, -0.5, 499.5);
  mClusterPatternIndex->SetStats(0);
  getObjectsManager()->startPublishing(mClusterPatternIndex.get());
  getObjectsManager()->setDisplayHint(mClusterPatternIndex.get(), "logy");

  mClusterSizeSummary = std::make_unique<TH1F>("mClusterSizeSummary", "Cluster Size Summary; Cluster Size (pixels);#Entries", 100, 0.5, 100.5);
  mClusterSizeSummary->SetStats(0);
  getObjectsManager()->startPublishing(mClusterSizeSummary.get());
  getObjectsManager()->setDisplayHint(mClusterSizeSummary.get(), "logy");

  mGroupedClusterSizeSummary = std::make_unique<TH1F>("mGroupedClusterSizeSummary", "Grouped Cluster Size Summary; Grouped Cluster Size (pixels);#Entries", 100, 0.5, 100.5);
  mGroupedClusterSizeSummary->SetStats(0);
  getObjectsManager()->startPublishing(mGroupedClusterSizeSummary.get());
  getObjectsManager()->setDisplayHint(mGroupedClusterSizeSummary.get(), "logy");

  mClusterPatternSensorIndices = std::make_unique<TH2F>("mClusterPatternSensorIndices",
                                                        "Cluster Pattern ID vs Chip ID;Chip ID;Pattern ID",
                                                        936, -0.5, 935.5, 500, -0.5, 499.5);
  mClusterPatternSensorIndices->SetStats(0);
  getObjectsManager()->startPublishing(mClusterPatternSensorIndices.get());
  getObjectsManager()->setDefaultDrawOptions(mClusterPatternSensorIndices.get(), "colz");

  mClusterOccupancySummary = std::make_unique<TH2F>("mClusterOccupancySummary", "Cluster Occupancy Summary;;",
                                                    10, -0.5, 9.5, 8, -0.5, 7.5);
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(1, "d0-f0");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(2, "d0-f1");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(3, "d1-f0");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(4, "d1-f1");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(5, "d2-f0");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(6, "d2-f1");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(7, "d3-f0");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(8, "d3-f1");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(9, "d4-f0");
  mClusterOccupancySummary->GetXaxis()->SetBinLabel(10, "d4-f1");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(1, "h0-z0");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(2, "h0-z1");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(3, "h0-z2");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(4, "h0-z3");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(5, "h1-z0");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(6, "h1-z1");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(7, "h1-z2");
  mClusterOccupancySummary->GetYaxis()->SetBinLabel(8, "h1-z3");
  mClusterOccupancySummary->SetStats(0);
  getObjectsManager()->startPublishing(mClusterOccupancySummary.get());
  getObjectsManager()->setDefaultDrawOptions(mClusterOccupancySummary.get(), "colz");

  mClusterZ = std::make_unique<TH1F>("mClusterZ", "Z position of clusters; Z (cm); # entries", 400, -80, -40);
  mClusterZ->SetStats(0);
  getObjectsManager()->startPublishing(mClusterZ.get());

  mClustersROFSize = std::make_unique<TH1F>("mClustersROFSize", "Distribution of the #clusters per ROF; # clusters per ROF; # entries", maxClusterROFSize, 0, maxClusterROFSize);
  mClustersROFSize->SetStats(0);
  getObjectsManager()->startPublishing(mClustersROFSize.get());
  getObjectsManager()->setDisplayHint(mClustersROFSize.get(), "logx logy");

  mNOfClustersTime = std::make_unique<TH1F>("mNOfClustersTime", "Number of clusters per time bin; time (s); # entries", NofTimeBins, 0, maxDuration);
  mNOfClustersTime->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mNOfClustersTime.get());

  mClustersBC = std::make_unique<TH1F>("mClustersBC", "Clusters per BC; BCid; # entries", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches);
  mClustersBC->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mClustersBC.get());
  getObjectsManager()->setDisplayHint(mClustersBC.get(), "hist");

  // define chip occupancy maps
  QcMFTUtilTables MFTTable;
  for (int iHalf = 0; iHalf < 2; iHalf++) {
    for (int iDisk = 0; iDisk < 5; iDisk++) {
      for (int iFace = 0; iFace < 2; iFace++) {
        int idx = (iDisk * 2 + iFace) + (10 * iHalf);
        auto chipmap = std::make_unique<TH2F>(
          Form("ChipOccupancyMaps/Half_%d/Disk_%d/Face_%d/mClusterChipOccupancyMap", iHalf, iDisk, iFace),
          Form("Cluster Chip Map h%d-d%d-f%d;x (cm);y (cm)", iHalf, iDisk, iFace),
          MFTTable.mNumberOfBinsInOccupancyMaps[idx][0],
          MFTTable.mNumberOfBinsInOccupancyMaps[idx][1],
          MFTTable.mNumberOfBinsInOccupancyMaps[idx][2],
          MFTTable.mNumberOfBinsInOccupancyMaps[idx][3],
          MFTTable.mNumberOfBinsInOccupancyMaps[idx][4],
          MFTTable.mNumberOfBinsInOccupancyMaps[idx][5]);
        chipmap->SetStats(0);
        mClusterChipOccupancyMap.push_back(std::move(chipmap));
        getObjectsManager()->startPublishing(mClusterChipOccupancyMap[idx].get());
        getObjectsManager()->setDefaultDrawOptions(mClusterChipOccupancyMap[idx].get(), "colz");
      } // loop over faces
    }   // loop over disks
  }     // loop over halfs

  for (auto nMFTLayer = 0; nMFTLayer < 10; nMFTLayer++) {
    auto clusterXY = std::make_unique<TH2F>(
      Form("ClusterXYinLayer/mClusterXYinLayer%d", nMFTLayer),
      Form("Cluster Position in Layer %d; x (cm); y (cm)", nMFTLayer), 400, -20, 20, 400, -20, 20);
    clusterXY->SetStats(0);
    mClusterXYinLayer.push_back(std::move(clusterXY));
    getObjectsManager()->startPublishing(mClusterXYinLayer[nMFTLayer].get());
    getObjectsManager()->setDefaultDrawOptions(mClusterXYinLayer[nMFTLayer].get(), "colz");

    auto clusterR = std::make_unique<TH1F>(Form("ClusterRinLayer/mClusterRinLayer%d", nMFTLayer), Form("Cluster Radial Position in Layer %d; r (cm); # entries", nMFTLayer), 400, 0, 20);
    mClusterRinLayer.push_back(std::move(clusterR));
    getObjectsManager()->startPublishing(mClusterRinLayer[nMFTLayer].get());
    getObjectsManager()->setDisplayHint(mClusterRinLayer[nMFTLayer].get(), "hist");
  }
}

void QcMFTClusterTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  // reset histograms
  reset();
}

void QcMFTClusterTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void QcMFTClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  if (mDict == nullptr) {
    ILOG(Info, Support) << "Getting dictionary from ccdb" << ENDM;
    auto mDictPtr = ctx.inputs().get<o2::itsmft::TopologyDictionary*>("cldict");
    mDict = mDictPtr.get();
    ILOG(Info, Support) << "Dictionary loaded with size: " << mDict->getSize() << ENDM;
  }

  // normalisation for the summary histogram to TF
  mClusterOccupancySummary->Fill(-1, -1);
  mClusterOccupancy->Fill(-1);
  mClusterPatternIndex->Fill(-1);
  mClusterSizeSummary->Fill(-1);
  mGroupedClusterSizeSummary->Fill(-1);

  // get the clusters
  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("randomcluster");
  const auto clustersROFs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");

  if (clusters.size() < 1)
    return;

  // get cluster patterns and iterator
  auto clustersPattern = ctx.inputs().get<gsl::span<unsigned char>>("patterns");
  auto patternIt = clustersPattern.begin();

  // get clusters with global xy position
  mClustersGlobal.clear();
  mClustersGlobal.reserve(clusters.size());
  o2::mft::ioutils::convertCompactClusters(clusters, patternIt, mClustersGlobal, mDict);

  // get correct timing info of the first TF orbit
  if (mRefOrbit == -1) {
    mRefOrbit = ctx.services().get<o2::framework::TimingInfo>().firstTForbit;
  }

  // reset the cluster pattern iterator which will be used later
  patternIt = clustersPattern.begin();

  // fill the clusters time histograms
  for (const auto& rof : clustersROFs) {
    mClustersROFSize->Fill(rof.getNEntries());
    float seconds = orbitToSeconds(rof.getBCData().orbit, mRefOrbit) + rof.getBCData().bc * o2::constants::lhc::LHCBunchSpacingNS * 1e-9;
    mNOfClustersTime->Fill(seconds, rof.getNEntries());
    mClustersBC->Fill(rof.getBCData().bc, rof.getNEntries());
  }

  // fill all other histograms
  for (auto& oneCluster : clusters) {
    int sensorID = oneCluster.getSensorID();
    int layerID = mMFTMapper.chip2Layer(sensorID); // used instead of previous layerID = mDisk[sensorID] * 2 + mFace[sensorID]
    (mHalf[sensorID] == 0) ? mClusterLayerIndexH0->Fill(layerID)
                           : mClusterLayerIndexH1->Fill(layerID);
    mClusterOccupancy->Fill(sensorID);
    mClusterPatternIndex->Fill(oneCluster.getPatternID());
    mClusterPatternSensorIndices->Fill(sensorID,
                                       oneCluster.getPatternID());

    if (oneCluster.getPatternID() != o2::itsmft::CompCluster::InvalidPatternID && !mDict->isGroup(oneCluster.getPatternID())) {
      mClusterSizeSummary->Fill(mDict->getNpixels(oneCluster.getPatternID()));
    } else {
      o2::itsmft::ClusterPattern patt(patternIt);
      mGroupedClusterSizeSummary->Fill(patt.getNPixels());
    }

    // fill occupancy maps
    int idx = layerID + (10 * mHalf[sensorID]);
    mClusterChipOccupancyMap[idx]->Fill(mX[sensorID], mY[sensorID]);

    // fill info into the summary histo
    int xBin = mDisk[sensorID] * 2 + mFace[sensorID];
    int yBin = mZone[sensorID] + mHalf[sensorID] * 4;
    mClusterOccupancySummary->Fill(xBin, yBin);
  }

  // fill the histograms that use global position of cluster
  for (auto& oneGlobalCluster : mClustersGlobal) {
    mClusterZ->Fill(oneGlobalCluster.getZ());
    int layerID = mMFTMapper.chip2Layer(oneGlobalCluster.getSensorID());
    mClusterXYinLayer[layerID]->Fill(oneGlobalCluster.getX(), oneGlobalCluster.getY());
    mClusterRinLayer[layerID]->Fill(std::sqrt(std::pow(oneGlobalCluster.getX(), 2) + std::pow(oneGlobalCluster.getY(), 2)));
  }
}

void QcMFTClusterTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void QcMFTClusterTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void QcMFTClusterTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mClusterOccupancy->Reset();
  mClusterPatternIndex->Reset();
  mClusterPatternSensorIndices->Reset();
  mClusterSizeSummary->Reset();
  mGroupedClusterSizeSummary->Reset();
  mClusterLayerIndexH0->Reset();
  mClusterLayerIndexH1->Reset();
  mClusterOccupancySummary->Reset();
  mClusterZ->Reset();
  mClustersROFSize->Reset();
  mNOfClustersTime->Reset();
  mClustersBC->Reset();
  for (int i = 0; i < 20; i++) {
    mClusterChipOccupancyMap[i]->Reset();
  }
  // layer histograms
  for (auto nMFTLayer = 0; nMFTLayer < 10; nMFTLayer++) { // there are 10 layers
    mClusterXYinLayer[nMFTLayer]->Reset();
    mClusterRinLayer[nMFTLayer]->Reset();
  }
}

void QcMFTClusterTask::getChipMapData()
{
  const o2::itsmft::ChipMappingMFT mapMFT;
  auto chipMapData = mapMFT.getChipMappingData();
  QcMFTUtilTables MFTTable;

  for (int i = 0; i < 936; i++) {
    mHalf[i] = chipMapData[i].half;
    mDisk[i] = chipMapData[i].disk;
    mFace[i] = (chipMapData[i].layer) % 2;
    mZone[i] = chipMapData[i].zone;
    mSensor[i] = chipMapData[i].localChipSWID;
    mTransID[i] = chipMapData[i].cable;
    mLadder[i] = MFTTable.mLadder[i];
    mX[i] = MFTTable.mX[i];
    mY[i] = MFTTable.mY[i];
  }
}

} // namespace o2::quality_control_modules::mft
