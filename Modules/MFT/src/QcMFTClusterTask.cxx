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
/// \author David Grund
/// \author Jakub Juracka
///

// C++
#include <gsl/span>
#include <string>
#include <vector>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TString.h>
#include <TAxis.h>
// O2
#include <DataFormatsITSMFT/CompCluster.h>
#include <Framework/InputRecord.h>
#include <Framework/TimingInfo.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <ITSMFTReconstruction/ChipMappingMFT.h>
#include <MFTTracking/IOUtils.h>
#include <CommonConstants/LHCConstants.h>
#include <DataFormatsITSMFT/ClusterPattern.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <Framework/ProcessingContext.h>
#include <Framework/ServiceRegistryRef.h>
#include <MFTTracking/Cluster.h>

// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTClusterTask.h"
#include "MFT/QcMFTUtilTables.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "DetectorsBase/GRPGeomHelper.h"

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

  // loading custom parameters
  mOnlineQC = 1;
  if (auto param = mCustomParameters.find("onlineQC"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - onlineQC: " << param->second << ENDM;
    mOnlineQC = stoi(param->second);
  }

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
  mClusterLayerIndexH0 = std::make_unique<TH1FRatio>(
    "mClusterLayerIndexH0", "Clusters per layer in H0;Layer;# entries per orbit", 10, -0.5, 9.5, true);
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
  getObjectsManager()->setDisplayHint(mClusterLayerIndexH0.get(), "hist");

  mClusterLayerIndexH1 = std::make_unique<TH1FRatio>(
    "mClusterLayerIndexH1", "Clusters per layer in H1;Layer;# entries per orbit", 10, -0.5, 9.5, true);
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
  getObjectsManager()->setDisplayHint(mClusterLayerIndexH1.get(), "hist");

  mClusterOccupancy = std::make_unique<TH1FRatio>(
    "mClusterOccupancy", "Chip Cluster Occupancy;Chip ID;# entries per orbit", 936, -0.5, 935.5, true);
  mClusterOccupancy->SetStats(0);
  getObjectsManager()->startPublishing(mClusterOccupancy.get());
  getObjectsManager()->setDisplayHint(mClusterOccupancy.get(), "hist");

  mClusterPatternIndex = std::make_unique<TH1FRatio>(
    "mClusterPatternIndex", "Cluster Pattern ID;Pattern ID;# entries per orbit", 500, -0.5, 499.5, true);
  mClusterPatternIndex->SetStats(0);
  getObjectsManager()->startPublishing(mClusterPatternIndex.get());
  getObjectsManager()->setDisplayHint(mClusterPatternIndex.get(), "hist logy");

  mClusterSizeSummary = std::make_unique<TH1FRatio>(
    "mClusterSizeSummary", "Cluster Size Summary; Cluster Size (pixels);# entries per orbit", 100, 0.5, 100.5, true);
  mClusterSizeSummary->SetStats(0);
  getObjectsManager()->startPublishing(mClusterSizeSummary.get());
  getObjectsManager()->setDisplayHint(mClusterSizeSummary.get(), "hist logy");

  mGroupedClusterSizeSummary = std::make_unique<TH1FRatio>(
    "mGroupedClusterSizeSummary", "Grouped Cluster Size Summary; Grouped Cluster Size (pixels);# entries per orbit", 100, 0.5, 100.5, true);
  mGroupedClusterSizeSummary->SetStats(0);
  getObjectsManager()->startPublishing(mGroupedClusterSizeSummary.get());
  getObjectsManager()->setDisplayHint(mGroupedClusterSizeSummary.get(), "hist logy");

  mClusterOccupancySummary = std::make_unique<TH2FRatio>("mClusterOccupancySummary",
                                                         "Cluster Occupancy Summary;;", 10, -0.5, 9.5, 8, -0.5, 7.5, true);
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

  mClusterZ = std::make_unique<TH1FRatio>(
    "mClusterZ", "Z position of clusters; Z (cm); # entries per orbit", 400, -80, -40, true);
  mClusterZ->SetStats(0);
  getObjectsManager()->startPublishing(mClusterZ.get());
  getObjectsManager()->setDisplayHint(mClusterZ.get(), "hist");

  mClustersROFSize = std::make_unique<TH1FRatio>(
    "mClustersROFSize", "Distribution of the #clusters per ROF; # clusters per ROF; # entries per orbit", QcMFTUtilTables::nROFBins, const_cast<float*>(QcMFTUtilTables::mROFBins), true);
  mClustersROFSize->SetStats(0);
  getObjectsManager()->startPublishing(mClustersROFSize.get());
  getObjectsManager()->setDisplayHint(mClustersROFSize.get(), "hist logx logy");

  mClustersBC = std::make_unique<TH1FRatio>(
    "mClustersBC", "Clusters per BC; BCid; # entries per orbit", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches, true);
  mClustersBC->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mClustersBC.get());
  getObjectsManager()->setDisplayHint(mClustersBC.get(), "hist");

  // only for online QC (disabled in A-QC)
  if (mOnlineQC == 1) {
    mClusterPatternSensorIndices = std::make_unique<TH2FRatio>(
      "mClusterPatternSensorIndices", "Cluster Pattern ID vs Chip ID;Chip ID;Pattern ID", 936, -0.5, 935.5, 500, -0.5, 499.5, true);
    mClusterPatternSensorIndices->SetStats(0);
    getObjectsManager()->startPublishing(mClusterPatternSensorIndices.get());
    getObjectsManager()->setDefaultDrawOptions(mClusterPatternSensorIndices.get(), "colz");

    // define chip occupancy maps
    QcMFTUtilTables MFTTable;
    for (int iHalf = 0; iHalf < 2; iHalf++) {
      for (int iDisk = 0; iDisk < 5; iDisk++) {
        for (int iFace = 0; iFace < 2; iFace++) {
          int idx = (iDisk * 2 + iFace) + (10 * iHalf);
          auto chipmap = std::make_unique<TH2FRatio>(
            Form("ChipOccupancyMaps/Half_%d/Disk_%d/Face_%d/mClusterChipOccupancyMap", iHalf, iDisk, iFace),
            Form("Cluster Chip Map h%d-d%d-f%d;x (cm);y (cm)", iHalf, iDisk, iFace),
            MFTTable.mNumberOfBinsInOccupancyMaps[idx][0],
            MFTTable.mNumberOfBinsInOccupancyMaps[idx][1],
            MFTTable.mNumberOfBinsInOccupancyMaps[idx][2],
            MFTTable.mNumberOfBinsInOccupancyMaps[idx][3],
            MFTTable.mNumberOfBinsInOccupancyMaps[idx][4],
            MFTTable.mNumberOfBinsInOccupancyMaps[idx][5], true);
          chipmap->SetStats(0);
          mClusterChipOccupancyMap.push_back(std::move(chipmap));
          getObjectsManager()->startPublishing(mClusterChipOccupancyMap[idx].get());
          getObjectsManager()->setDefaultDrawOptions(mClusterChipOccupancyMap[idx].get(), "colz");
        } // loop over faces
      } // loop over disks
    } // loop over halfs

    // layer histograms
    for (auto nMFTLayer = 0; nMFTLayer < 10; nMFTLayer++) {
      auto clusterXY = std::make_unique<TH2FRatio>(
        Form("ClusterXYinLayer/mClusterXYinLayer%d", nMFTLayer),
        Form("Cluster Position in Layer %d; x (cm); y (cm)", nMFTLayer), 400, -20, 20, 400, -20, 20, true);
      clusterXY->SetStats(0);
      mClusterXYinLayer.push_back(std::move(clusterXY));
      getObjectsManager()->startPublishing(mClusterXYinLayer[nMFTLayer].get());
      getObjectsManager()->setDefaultDrawOptions(mClusterXYinLayer[nMFTLayer].get(), "colz");

      auto clusterR = std::make_unique<TH1FRatio>(
        Form("ClusterRinLayer/mClusterRinLayer%d", nMFTLayer),
        Form("Cluster Radial Position in Layer %d; r (cm); # entries", nMFTLayer), 400, 0, 20, true);
      mClusterRinLayer.push_back(std::move(clusterR));
      getObjectsManager()->startPublishing(mClusterRinLayer[nMFTLayer].get());
      getObjectsManager()->setDisplayHint(mClusterRinLayer[nMFTLayer].get(), "hist");
    }
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
  auto mNOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();

  if (mDict == nullptr) {
    ILOG(Info, Support) << "Getting dictionary from ccdb" << ENDM;
    auto mDictPtr = ctx.inputs().get<o2::itsmft::TopologyDictionary*>("cldict");
    mDict = mDictPtr.get();
    ILOG(Info, Support) << "Dictionary loaded with size: " << mDict->getSize() << ENDM;
  }
  if (!mGeom) {
    o2::mft::GeometryTGeo::adopt(TaskInterface::retrieveConditionAny<o2::mft::GeometryTGeo>("MFT/Config/Geometry"));
    mGeom = o2::mft::GeometryTGeo::Instance();
    ILOG(Info, Support) << "GeometryTGeo loaded" << ENDM;
  }

  // get the clusters
  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("randomcluster");
  const auto clustersROFs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");

  if (clusters.empty()) {
    return;
  }

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
    mClustersROFSize->getNum()->Fill(rof.getNEntries());
    mClustersBC->getNum()->Fill(rof.getBCData().bc, rof.getNEntries());
  }

  // fill all other histograms
  for (auto& oneCluster : clusters) {
    int sensorID = oneCluster.getSensorID();
    int layerID = mMFTMapper.chip2Layer(sensorID); // used instead of previous layerID = mDisk[sensorID] * 2 + mFace[sensorID]
    (mHalf[sensorID] == 0) ? mClusterLayerIndexH0->getNum()->Fill(layerID)
                           : mClusterLayerIndexH1->getNum()->Fill(layerID);
    mClusterOccupancy->getNum()->Fill(sensorID);
    mClusterPatternIndex->getNum()->Fill(oneCluster.getPatternID());
    if (mOnlineQC == 1)
      mClusterPatternSensorIndices->getNum()->Fill(sensorID, oneCluster.getPatternID());

    if (oneCluster.getPatternID() != o2::itsmft::CompCluster::InvalidPatternID && !mDict->isGroup(oneCluster.getPatternID())) {
      mClusterSizeSummary->getNum()->Fill(mDict->getNpixels(oneCluster.getPatternID()));
    } else {
      o2::itsmft::ClusterPattern patt(patternIt);
      mGroupedClusterSizeSummary->getNum()->Fill(patt.getNPixels());
    }

    // fill occupancy maps
    int idx = layerID + (10 * mHalf[sensorID]);
    if (mOnlineQC == 1)
      mClusterChipOccupancyMap[idx]->getNum()->Fill(mX[sensorID], mY[sensorID]);

    // fill info into the summary histo
    int xBin = mDisk[sensorID] * 2 + mFace[sensorID];
    int yBin = mZone[sensorID] + mHalf[sensorID] * 4;
    mClusterOccupancySummary->getNum()->Fill(xBin, yBin);
  }

  // fill the histograms that use global position of cluster
  for (auto& oneGlobalCluster : mClustersGlobal) {
    mClusterZ->getNum()->Fill(oneGlobalCluster.getZ());
    int layerID = mMFTMapper.chip2Layer(oneGlobalCluster.getSensorID());
    if (mOnlineQC == 1) {
      mClusterXYinLayer[layerID]->getNum()->Fill(oneGlobalCluster.getX(), oneGlobalCluster.getY());
      mClusterRinLayer[layerID]->getNum()->Fill(std::sqrt(std::pow(oneGlobalCluster.getX(), 2) + std::pow(oneGlobalCluster.getY(), 2)));
    }
  }

  // fill the denominators
  mClusterLayerIndexH0->getDen()->SetBinContent(1, mClusterLayerIndexH0->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClusterLayerIndexH1->getDen()->SetBinContent(1, mClusterLayerIndexH1->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClusterOccupancy->getDen()->SetBinContent(1, mClusterOccupancy->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClusterPatternIndex->getDen()->SetBinContent(1, mClusterPatternIndex->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClusterSizeSummary->getDen()->SetBinContent(1, mClusterSizeSummary->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mGroupedClusterSizeSummary->getDen()->SetBinContent(1, mGroupedClusterSizeSummary->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClusterOccupancySummary->getDen()->SetBinContent(1, 1, mClusterOccupancySummary->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
  mClusterZ->getDen()->SetBinContent(1, mClusterZ->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClustersROFSize->getDen()->SetBinContent(1, mClustersROFSize->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mClustersBC->getDen()->SetBinContent(1, mClustersBC->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  if (mOnlineQC == 1) {
    mClusterPatternSensorIndices->getDen()->SetBinContent(1, 1, mClusterPatternSensorIndices->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
    for (int i = 0; i < 20; i++)
      mClusterChipOccupancyMap[i]->getDen()->SetBinContent(1, 1, mClusterChipOccupancyMap[i]->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
    // layer histograms
    for (auto nMFTLayer = 0; nMFTLayer < 10; nMFTLayer++) { // there are 10 layers
      mClusterXYinLayer[nMFTLayer]->getDen()->SetBinContent(1, 1, mClusterXYinLayer[nMFTLayer]->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
      mClusterRinLayer[nMFTLayer]->getDen()->SetBinContent(1, mClusterRinLayer[nMFTLayer]->getDen()->GetBinContent(1) + mNOrbitsPerTF);
    }
  }
}

void QcMFTClusterTask::endOfCycle()
{
  // update all THRatios
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  mClusterLayerIndexH0->update();
  mClusterLayerIndexH1->update();
  mClusterOccupancy->update();
  mClusterPatternIndex->update();
  mClusterSizeSummary->update();
  mGroupedClusterSizeSummary->update();
  mClusterOccupancySummary->update();
  mClusterZ->update();
  mClustersROFSize->update();
  mClustersBC->update();
  if (mOnlineQC == 1) {
    mClusterPatternSensorIndices->update();
    for (int i = 0; i < 20; i++)
      mClusterChipOccupancyMap[i]->update();
    // layer histograms
    for (auto nMFTLayer = 0; nMFTLayer < 10; nMFTLayer++) { // there are 10 layers
      mClusterXYinLayer[nMFTLayer]->update();
      mClusterRinLayer[nMFTLayer]->update();
    }
  }
}

void QcMFTClusterTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void QcMFTClusterTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mClusterLayerIndexH0->Reset();
  mClusterLayerIndexH1->Reset();
  mClusterOccupancy->Reset();
  mClusterPatternIndex->Reset();
  mClusterSizeSummary->Reset();
  mGroupedClusterSizeSummary->Reset();
  mClusterOccupancySummary->Reset();
  mClusterZ->Reset();
  mClustersROFSize->Reset();
  mClustersBC->Reset();
  if (mOnlineQC == 1) {
    mClusterPatternSensorIndices->Reset();
    for (int i = 0; i < 20; i++)
      mClusterChipOccupancyMap[i]->Reset();
    // layer histograms
    for (auto nMFTLayer = 0; nMFTLayer < 10; nMFTLayer++) { // there are 10 layers
      mClusterXYinLayer[nMFTLayer]->Reset();
      mClusterRinLayer[nMFTLayer]->Reset();
    }
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
