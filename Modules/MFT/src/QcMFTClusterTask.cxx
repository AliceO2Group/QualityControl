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
#include <DataFormatsITSMFT/ROFRecord.h>
#include <ITSMFTReconstruction/ChipMappingMFT.h>

// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTClusterTask.h"
#include "MFT/QcMFTUtilTables.h"

// C++
#include <fstream>

namespace o2::quality_control_modules::mft
{

QcMFTClusterTask::~QcMFTClusterTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize QcMFTClusterTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
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

  mClusterPatternIndex = std::make_unique<TH1F>("mClusterPatternIndex", "Cluster Pattern ID;Pattern ID;#Entries per TF", 300, -0.5, 299.5);
  mClusterPatternIndex->SetStats(0);
  getObjectsManager()->startPublishing(mClusterPatternIndex.get());
  

  mClusterPatternSensorIndices = std::make_unique<TH2F>("mClusterPatternSensorIndices",
                                                        "Cluster Pattern ID vs Chip ID;Chip ID;Pattern ID",
                                                        936, -0.5, 935.5, 100, -0.5, 99.5);
  mClusterPatternSensorIndices->SetStats(0);
  mClusterPatternSensorIndices->SetOption("colz");
  getObjectsManager()->startPublishing(mClusterPatternSensorIndices.get());

  mClusterOccupancySummary = std::make_unique<TH2F>("mClusterOccupancySummary", "mClusterOccupancySummary",
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
  mClusterOccupancySummary->SetOption("colz");
  mClusterOccupancySummary->SetStats(0);
  getObjectsManager()->startPublishing(mClusterOccupancySummary.get());

  // --Ladder occupancy maps
  //==============================================
  for (int i = 0; i < 280; i++) { // there are 280 ladders
    auto ladderHistogram = std::make_unique<TH2F>(
      Form("LadderClusterSensorMaps/Ladder%d-h%d-d%d-f%d-z%d", i, mHalfLadder[i], mDiskLadder[i], mFaceLadder[i], mZoneLadder[i]),
      Form("Cluster sensor map ladder%d-h%d-d%d-f%d-z%d; Pattern ID; Chip", i, mHalfLadder[i], mDiskLadder[i], mFaceLadder[i], mZoneLadder[i]),
      100, 0, 100, // Pattern ID per chip
      mChipsInLadder[i], 0, mChipsInLadder[i]);
    for (int iBin = 0; iBin < mChipsInLadder[i]; iBin++)
      ladderHistogram->GetYaxis()->SetBinLabel(iBin + 1, Form("%d", iBin));
    ladderHistogram->SetStats(0);
    ladderHistogram->SetOption("colz");
    mClusterLadderPatternSensorMap.push_back(std::move(ladderHistogram));
    getObjectsManager()->startPublishing(mClusterLadderPatternSensorMap[i].get());
  }

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
        chipmap->SetOption("colz");
        mClusterChipOccupancyMap.push_back(std::move(chipmap));
        getObjectsManager()->startPublishing(mClusterChipOccupancyMap[idx].get());
      } // loop over faces
    }   // loop over disks
  }     // loop over halfs
}

void QcMFTClusterTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;

  // reset histograms
  reset();
}

void QcMFTClusterTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void QcMFTClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // normalisation for the Summary histogram to TF
  mClusterOccupancySummary->Fill(-1, -1);
  mClusterOccupancy->Fill(-1);
  mClusterPatternIndex->Fill(-1);

  // get the clusters
  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("randomcluster");
  if (clusters.size() < 1)
    return;

  // fill the histograms
  for (auto& oneCluster : clusters) {
    int sensorID = oneCluster.getSensorID();
    int layerID = mDisk[sensorID] * 2 + mFace[sensorID];
    (mHalf[sensorID] == 0) ? mClusterLayerIndexH0->Fill(layerID)
                           : mClusterLayerIndexH1->Fill(layerID);
    mClusterOccupancy->Fill(sensorID);
    mClusterPatternIndex->Fill(oneCluster.getPatternID());
    mClusterPatternSensorIndices->Fill(sensorID,
                                       oneCluster.getPatternID());

    // fill occupancy maps
    int idx = layerID + (10 * mHalf[sensorID]);
    mClusterChipOccupancyMap[idx]->Fill(mX[sensorID], mY[sensorID]);

    // fill info into the summary histo
    int xBin = mDisk[sensorID] * 2 + mFace[sensorID];
    int yBin = mZone[sensorID] + mHalf[sensorID] * 4;
    mClusterOccupancySummary->Fill(xBin, yBin);

    // fill ladder histogram
    mClusterLadderPatternSensorMap[mChipLadder[sensorID]]->Fill(oneCluster.getPatternID() >> 1, mChipPositionInLadder[sensorID]);
  }
}

void QcMFTClusterTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void QcMFTClusterTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void QcMFTClusterTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mClusterOccupancy->Reset();
  mClusterPatternIndex->Reset();
  mClusterPatternSensorIndices->Reset();
  mClusterLayerIndexH0->Reset();
  mClusterLayerIndexH1->Reset();
  mClusterOccupancySummary->Reset();
  for (int i = 0; i < 20; i++) {
    mClusterChipOccupancyMap[i]->Reset();
  }
  // ladder histograms
  for (int i = 0; i < 280; i++) { // there are 280 ladders
    mClusterLadderPatternSensorMap[i]->Reset();
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
    // info needed for ladder histograms
    mChipLadder[i] = chipMapData[i].module;
    mChipPositionInLadder[i] = chipMapData[i].chipOnModule;
    if (mChipsInLadder[mChipLadder[i]] < (mChipPositionInLadder[i] + 1))
      mChipsInLadder[mChipLadder[i]]++;
    mHalfLadder[mChipLadder[i]] = mHalf[i];
    mDiskLadder[mChipLadder[i]] = mDisk[i];
    mFaceLadder[mChipLadder[i]] = mFace[i];
    mZoneLadder[mChipLadder[i]] = mZone[i];
  }
}

} // namespace o2::quality_control_modules::mft
