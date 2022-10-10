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
/// \file   ClusterTask.cxx
/// \author Vivek Kumar Singh
///

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iostream>

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "EMCAL/ClusterTask.h"
#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

#include <gsl/span>

#include "DataFormatsEMCAL/Digit.h"
#include "DataFormatsEMCAL/Cluster.h"
#include "DataFormatsEMCAL/TriggerRecord.h"
#include "DetectorsBase/GeometryManager.h"
#include "EMCALBase/Geometry.h"
#include "EMCALCalib/CalibDB.h"
#include "EMCALCalib/BadChannelMap.h"
#include "EMCALCalib/GainCalibrationFactors.h"
#include "EMCALCalib/TimeCalibrationParams.h"
#include "Framework/ControlService.h"
#include "Framework/Logger.h"
#include <TGeoManager.h>

#include <DataFormatsEMCAL/EventHandler.h>
#include <EMCALBase/ClusterFactory.h>

#include <DataFormatsEMCAL/AnalysisCluster.h>

#include <EMCALReconstruction/Clusterizer.h> //svk

namespace o2::quality_control_modules::emcal
{

ClusterTask::~ClusterTask()
{
  auto conditionalDelete = [](auto obj) {
    if (obj) {
      delete obj;
    }
  };

  conditionalDelete(mHistNclustPerTF);
  conditionalDelete(mHistNclustPerEvt);
  conditionalDelete(mHistClustEtaPhi);

  conditionalDelete(mHistTime_EMCal);
  conditionalDelete(mHistClustE_EMCal);
  conditionalDelete(mHistNCells_EMCal);
  conditionalDelete(mHistM02_EMCal);
  conditionalDelete(mHistM20_EMCal);
  conditionalDelete(mHistM02VsClustE__EMCal);
  conditionalDelete(mHistM20VsClustE__EMCal);

  conditionalDelete(mHistTime_DCal);
  conditionalDelete(mHistClustE_DCal);
  conditionalDelete(mHistNCells_DCal);
  conditionalDelete(mHistM02_DCal);
  conditionalDelete(mHistM20_DCal);
  conditionalDelete(mHistM02VsClustE__DCal);
  conditionalDelete(mHistM20VsClustE__DCal);
}

void ClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::setDetector("EMC");                        // svk
  ILOG(Info, Support) << "initialize ClusterTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  auto get_bool = [](const std::string_view input) -> bool { // svk
    return input == "true";
  };

  if (hasConfigValue("useInternalClusterizer")) {
    mInternalClusterizer = get_bool(getConfigValueLower("useInternalClusterizer"));
    if (mInternalClusterizer) {
      ILOG(Info, Support) << "Enabling internal clusterizer . . . " << ENDM;
      // Check whether we want to run the calibration before (only in case we run the internal clusterizer)
      if (hasConfigValue("calibrateCells")) {
        mCalibrate = get_bool(getConfigValueLower("calibrateCells"));
        if (mCalibrate) {
          ILOG(Info, Support) << "Calibrate cells before clusterization" << ENDM;
        }
      }

      /// Configure clusterizer settings
      configureClusterizerSettings();
    }
  }

  mEventHandler = std::make_unique<o2::emcal::EventHandler<o2::emcal::Cell>>();
  mClusterFactory = std::make_unique<o2::emcal::ClusterFactory<o2::emcal::Cell>>();

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000); // svk
  mClusterFactory->setGeometry(mGeometry);

  mHistNclustPerTF = new TH1F("NclustPerTF", "Number of clusters per time frame;N_{Cluster}/TF;", 10, 0.0, 10.0); // svk
  getObjectsManager()->startPublishing(mHistNclustPerTF);

  mHistNclustPerEvt = new TH1F("NclustPerEvt", "Number of clusters per event;N_{Cluster}/Event;", 100, 0.0, 100.0); // svk
  getObjectsManager()->startPublishing(mHistNclustPerEvt);

  mHistClustEtaPhi = new TH2F("ClustEtaPhi", "Cluster #eta and #phi distribution;#eta;#phi", 100, -1.0, 1.0, 100, 0.0, 2 * TMath::Pi()); // svk
  getObjectsManager()->startPublishing(mHistClustEtaPhi);

  ///////////////////
  // EMCal histograms/
  ///////////////////
  mHistTime_EMCal = new TH2F("Time_EMCal", "Time of clusters;Clust E(GeV);t(ns);", 500, 0, 50, 1800, -900, 900); // svk
  getObjectsManager()->startPublishing(mHistTime_EMCal);

  mHistClustE_EMCal = new TH1F("ClustE_EMCal", "Cluster energy distribution; Cluster E(GeV);Counts", 500, 0.0, 50.0);
  getObjectsManager()->startPublishing(mHistClustE_EMCal);

  mHistNCells_EMCal = new TH2F("Cells_EMCal", "No of cells in a cluster;Cluster E(GeV);N^{EMC}_{cells}", 500, 0, 50, 30, 0, 30);
  getObjectsManager()->startPublishing(mHistNCells_EMCal);

  mHistM02_EMCal = new TH1F("M02_EMCal", "M02 distribution; M02;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM02_EMCal);

  mHistM20_EMCal = new TH1F("M20_EMCal", "M20 distribution; M20;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM20_EMCal);

  mHistM02VsClustE__EMCal = new TH2F("M02_Vs_ClustE_EMCal", "M02 Vs Cluster Energy in EMCal;Cluster E(GeV);M02", 500, 0, 50.0, 200, 0.0, 2.0); // svk
  getObjectsManager()->startPublishing(mHistM02VsClustE__EMCal);

  mHistM20VsClustE__EMCal = new TH2F("M20_Vs_ClustE_EMCal", "M20 Vs Cluster Energy in EMCal;Cluster E(GeV);M02", 500, 0, 50.0, 200, 0.0, 2.0); // svk
  getObjectsManager()->startPublishing(mHistM20VsClustE__EMCal);

  ///////////////////
  // DCal histograms//
  ///////////////////

  mHistTime_DCal = new TH2F("Time_DCal", "Time of clusters;Clust E(GeV);t(ns);", 500, 0, 50, 1800, -900, 900); // svk
  getObjectsManager()->startPublishing(mHistTime_DCal);

  mHistClustE_DCal = new TH1F("ClustE_DCal", "Cluster energy distribution; Cluster E(GeV);Counts", 500, 0.0, 50.0);
  getObjectsManager()->startPublishing(mHistClustE_DCal);

  mHistNCells_DCal = new TH2F("Cells_DCal", "No of cells in a cluster;Cluster E;N^{EMC}_{cells}", 500, 0, 50, 30, 0, 30);
  getObjectsManager()->startPublishing(mHistNCells_DCal);

  mHistM02_DCal = new TH1F("M02_DCal", "M02 distribution; M02;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM02_DCal);

  mHistM20_DCal = new TH1F("M20_DCal", "M20 distribution; M20;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM20_DCal);

  mHistM02VsClustE__DCal = new TH2F("M02_Vs_ClustE_DCal", "M02 Vs Cluster Energy in DCal;Cluster E(GeV);M02", 500, 0, 50.0, 200, 0.0, 2.0); // svk
  getObjectsManager()->startPublishing(mHistM02VsClustE__DCal);

  mHistM20VsClustE__DCal = new TH2F("M20_Vs_ClustE_DCal", "M20 Vs Cluster Energy in DCal;Cluster E(GeV);M02", 500, 0, 50.0, 200, 0.0, 2.0); // svk
  getObjectsManager()->startPublishing(mHistM20VsClustE__DCal);
}

void ClusterTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  resetHistograms();
}

void ClusterTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;

  if (!o2::base::GeometryManager::isGeometryLoaded()) {
    ILOG(Info, Support) << "Loading geometry" << ENDM;
    TaskInterface::retrieveConditionAny<TObject>("GLO/Config/Geometry");
    ILOG(Info, Support) << "Geometry loaded" << ENDM;
  }

  // Loading EMCAL calibration objects
  if (mCalibrate) {
    std::map<std::string, std::string> metadata;
    mBadChannelMap = retrieveConditionAny<o2::emcal::BadChannelMap>(o2::emcal::CalibDB::getCDBPathBadChannelMap(), metadata);
    if (!mBadChannelMap) {
      ILOG(Error, Support) << "No bad channel map - bad channel removal before recalibration will not be possible " << ENDM;
    }

    mTimeCalib = retrieveConditionAny<o2::emcal::TimeCalibrationParams>(o2::emcal::CalibDB::getCDBPathTimeCalibrationParams(), metadata);
    if (!mTimeCalib) {
      ILOG(Info, Support) << " No time calibration params - time recalibration before clusterization will not be possible " << ENDM;
    }

    mEnergyCalib = retrieveConditionAny<o2::emcal::GainCalibrationFactors>(o2::emcal::CalibDB::getCDBPathGainCalibrationParams(), metadata);
    if (!mEnergyCalib) {
      ILOG(Error, Support) << "No energy calibration params - energy recalibration before clusterization will not be possible" << ENDM;
    }
  }
}

void ClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::string inputCell = "emcal-cells";
  std::string inputCellTR = "emcal-cellstriggerecords";

  auto cell = ctx.inputs().get<gsl::span<o2::emcal::Cell>>(inputCell.c_str());
  auto cellTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(inputCellTR.c_str());

  if (mInternalClusterizer) {
    std::vector<o2::emcal::Cluster> cluster;
    std::vector<int> cellIndex;
    std::vector<o2::emcal::TriggerRecord> clusterTR, cellIndexTR;

    // prepare optional recalibration before clusterization
    std::vector<o2::emcal::Cell> calibratedCells;
    std::vector<o2::emcal::TriggerRecord> calibratedTriggerRecords;
    gsl::span<const o2::emcal::Cell> inputcells;
    gsl::span<const o2::emcal::TriggerRecord> inputTriggerRecords;
    if (mCalibrate) {
      // build recalibrated cell collection;
      ILOG(Debug, Support) << "Calibrate cells" << ENDM;
      getCalibratedCells(cell, cellTR, calibratedCells, calibratedTriggerRecords);
      inputcells = gsl::span<const o2::emcal::Cell>(calibratedCells);
      inputTriggerRecords = gsl::span<const o2::emcal::TriggerRecord>(calibratedTriggerRecords);
    } else {
      // No calibration performed, forward external cell collection
      ILOG(Debug, Support) << "No calibration performed - forward containers" << ENDM;
      inputcells = cell;
      inputTriggerRecords = cellTR;
    }

    ILOG(Debug, Support) << "Received " << cell.size() << " cells from " << cellTR.size() << " triggers, " << inputcells.size() << " cells from " << inputTriggerRecords.size() << " triggers to clusterizer" << ENDM;

    findClustersInternal(inputcells, inputTriggerRecords, cluster, clusterTR, cellIndex, cellIndexTR); // do internal clustering here
    ILOG(Debug, Support) << "Found  " << cluster.size() << " CLusters  " << ENDM;
    ILOG(Debug, Support) << "Found  " << clusterTR.size() << " Cluster Trigger Records  " << ENDM;
    ILOG(Debug, Support) << "Found  " << cellIndex.size() << " Cell Index  " << ENDM;
    ILOG(Debug, Support) << "Found  " << cellIndexTR.size() << " Cell Index Records " << ENDM;

    analyseTimeframe(inputcells, inputTriggerRecords, cluster, clusterTR, cellIndex, cellIndexTR); // Fill histos

  } else {

    std::string inputCluster = "emcal-clusters";
    std::string inputClusterTR = "emcal-clustertriggerecords";

    std::string inputCellIndex = "emcal-cellindices";
    std::string inputCellIndexTR = "emcal-citriggerecords";

    auto cluster = ctx.inputs().get<gsl::span<o2::emcal::Cluster>>(inputCluster.c_str());
    auto clusterTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(inputClusterTR.c_str());
    auto cellIndex = ctx.inputs().get<gsl::span<int>>(inputCellIndex.c_str());
    auto cellIndexTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(inputCellIndexTR.c_str());

    analyseTimeframe(cell, cellTR, cluster, clusterTR, cellIndex, cellIndexTR); // Fill histos
  }
}

void ClusterTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ClusterTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

//_____________________________  Fill function _____________________________
void ClusterTask::analyseTimeframe(const gsl::span<const o2::emcal::Cell>& cells, const gsl::span<const o2::emcal::TriggerRecord>& cellTriggerRecords, const gsl::span<const o2::emcal::Cluster> clusters, const gsl::span<const o2::emcal::TriggerRecord> clusterTriggerRecords, const gsl::span<const int> clusterIndices, const gsl::span<const o2::emcal::TriggerRecord> cellIndexTriggerRecords)
{

  mEventHandler->reset();
  mClusterFactory->reset();

  mEventHandler->setClusterData(clusters, clusterIndices, clusterTriggerRecords, cellIndexTriggerRecords);
  mEventHandler->setCellData(cells, cellTriggerRecords);

  mHistNclustPerTF->Fill(clusters.size());               // svk
  mHistNclustPerEvt->Fill(clusterTriggerRecords.size()); // svk

  for (int iev = 0; iev < mEventHandler->getNumberOfEvents(); iev++) {
    auto inputEvent = mEventHandler->buildEvent(iev);

    mClusterFactory->setClustersContainer(inputEvent.mClusters);
    mClusterFactory->setCellsContainer(inputEvent.mCells);
    mClusterFactory->setCellsIndicesContainer(inputEvent.mCellIndices);

    for (int icl = 0; icl < mClusterFactory->getNumberOfClusters(); icl++) {

      o2::emcal::AnalysisCluster analysisCluster = mClusterFactory->buildCluster(icl);

      Double_t clustE = analysisCluster.E();
      Double_t clustT = analysisCluster.getClusterTime(); // svk converted to ns adapted from Run2;

      /////////////////////////////////
      // Select EMCAL or DCAL clusters//
      /////////////////////////////////
      Bool_t clsTypeEMC;

      math_utils::Point3D<float> emcx = analysisCluster.getGlobalPosition();

      TVector3 clustpos(emcx.X(), emcx.Y(), emcx.Z());
      Double_t emcphi = TVector2::Phi_0_2pi(clustpos.Phi());
      Double_t emceta = clustpos.Eta();

      mHistClustEtaPhi->Fill(emceta, emcphi); // svk

      if (emcphi < 4.)
        clsTypeEMC = kTRUE; // EMCAL : 80 < phi < 187
      else
        clsTypeEMC = kFALSE; // DCAL  : 260 < phi < 327

      if (clsTypeEMC) {
        mHistTime_EMCal->Fill(clustE, clustT); // svk
        mHistClustE_EMCal->Fill(clustE);
        mHistNCells_EMCal->Fill(clustE, analysisCluster.getNCells());
        mHistM02_EMCal->Fill(analysisCluster.getM02());
        mHistM20_EMCal->Fill(analysisCluster.getM20());
        mHistM02VsClustE__EMCal->Fill(clustE, analysisCluster.getM02()); // svk
        mHistM20VsClustE__EMCal->Fill(clustE, analysisCluster.getM20()); // svk
      } else {
        mHistTime_DCal->Fill(clustE, clustT); // svk
        mHistClustE_DCal->Fill(clustE);
        mHistNCells_DCal->Fill(clustE, analysisCluster.getNCells());
        mHistM02_DCal->Fill(analysisCluster.getM02());
        mHistM20_DCal->Fill(analysisCluster.getM20());
        mHistM02VsClustE__DCal->Fill(clustE, analysisCluster.getM02()); // svk
        mHistM20VsClustE__DCal->Fill(clustE, analysisCluster.getM20()); // svk
      }

    } // cls loop

  } // event loop
}

//_____________________________  Internal clusteriser function _____________________________
void ClusterTask::findClustersInternal(const gsl::span<const o2::emcal::Cell>& cells, const gsl::span<const o2::emcal::TriggerRecord>& cellTriggerRecords, std::vector<o2::emcal::Cluster>& clusters, std::vector<o2::emcal::TriggerRecord>& clusterTriggerRecords, std::vector<int>& clusterIndices, std::vector<o2::emcal::TriggerRecord>& clusterIndexTriggerRecords)
{
  LOG(debug) << "[EMCALClusterizer - findClustersInternal] called";
  // mTimer.Start(false);

  if (!mClusterizer) {
    mClusterizer = std::make_unique<o2::emcal::Clusterizer<o2::emcal::Cell>>();
    ILOG(Info, Support) << "Internal clusterizer initialized" << ENDM;
    mClusterizer->setGeometry(mGeometry); // svk - set geometry for clusterizer

    // Initialize clusterizer from clusterizer params
    ILOG(Info, Support) << mClusterizerSettings << ENDM;
    mClusterizer->initialize(mClusterizerSettings.mMaxTimeDeltaCells, mClusterizerSettings.mMinCellTime, mClusterizerSettings.mMaxCellTime, mClusterizerSettings.mGradientCut, mClusterizerSettings.mDoEnergyGradientCut, mClusterizerSettings.mSeedThreshold, mClusterizerSettings.mCellThreshold);
  }

  clusters.clear();
  clusterTriggerRecords.clear();
  clusterIndices.clear();
  clusterIndexTriggerRecords.clear();

  int currentStartClusters = 0; // clusters->size();  //svk
  int currentStartIndices = 0;  // clusterIndices->size();  //svk

  for (const auto& iTrgRcrd : cellTriggerRecords) {
    LOG(debug) << " findClustersInternal loop over iTrgRcrd  " << ENDM;

    mClusterizer->clear();
    if (cells.size() && iTrgRcrd.getNumberOfObjects()) {
      LOG(debug) << " Number of cells put in " << cells.size() << ENDM;
      mClusterizer->findClusters(gsl::span<const o2::emcal::Cell>(cells.data() + iTrgRcrd.getFirstEntry(), iTrgRcrd.getNumberOfObjects())); // Find clusters on cells/digits (pass by ref)
    }

    auto outputClustersTemp = mClusterizer->getFoundClusters();
    LOG(debug) << " Number of Clusters in outputClustersTemp " << outputClustersTemp->size() << ENDM;

    auto outputCellDigitIndicesTemp = mClusterizer->getFoundClustersInputIndices();

    std::copy(outputClustersTemp->begin(), outputClustersTemp->end(), std::back_inserter(clusters));
    std::copy(outputCellDigitIndicesTemp->begin(), outputCellDigitIndicesTemp->end(), std::back_inserter(clusterIndices));

    clusterTriggerRecords.emplace_back(iTrgRcrd.getBCData(), currentStartClusters, outputClustersTemp->size());             // svk
    clusterIndexTriggerRecords.emplace_back(iTrgRcrd.getBCData(), currentStartIndices, outputCellDigitIndicesTemp->size()); // svk

    currentStartClusters = clusters.size();      // svk
    currentStartIndices = clusterIndices.size(); // svk
  }
  LOG(debug) << "[EMCALClusterizer - findClustersInternal] Writing " << clusters.size() << " clusters ...";
  // mTimer.Stop();
}

void ClusterTask::getCalibratedCells(const gsl::span<const o2::emcal::Cell>& cells, const gsl::span<const o2::emcal::TriggerRecord>& triggerRecords, std::vector<o2::emcal::Cell>& calibratedCells, std::vector<o2::emcal::TriggerRecord>& calibratedTriggerRecords)
{
  int currentlast = 0;
  for (const auto& trg : triggerRecords) {
    if (!trg.getNumberOfObjects()) {
      // No EMCAL cells in event - post empty trigger record
      o2::emcal::TriggerRecord nexttrigger(trg.getBCData(), currentlast, 0);
      nexttrigger.setTriggerBits(trg.getTriggerBits());
      calibratedTriggerRecords.push_back(nexttrigger);
      continue;
    }
    for (const auto& inputcell : gsl::span<const o2::emcal::Cell>(cells.data() + trg.getFirstEntry(), trg.getNumberOfObjects())) {
      if (mBadChannelMap) {
        if (mBadChannelMap->getChannelStatus(inputcell.getTower()) != o2::emcal::BadChannelMap::MaskType_t::GOOD_CELL) {
          // Cell marked as bad or warm, continue
          continue;
        }
      }
      auto cellamplitude = inputcell.getAmplitude();
      auto celltime = inputcell.getTimeStamp();
      if (mTimeCalib) {
        celltime -= mTimeCalib->getTimeCalibParam(inputcell.getTower(), inputcell.getLowGain());
      }
      if (mEnergyCalib) {
        cellamplitude *= mEnergyCalib->getGainCalibFactors(inputcell.getTower());
      }
      calibratedCells.emplace_back(inputcell.getTower(), cellamplitude, celltime, inputcell.getType());
    }
    int ncellsEvent = calibratedCells.size() - currentlast;
    o2::emcal::TriggerRecord nexttrigger(trg.getBCData(), currentlast, ncellsEvent);
    nexttrigger.setTriggerBits(trg.getTriggerBits());
    calibratedTriggerRecords.push_back(nexttrigger);
  }
}

void ClusterTask::configureClusterizerSettings()
{
  auto get_bool = [](const std::string_view input) -> bool {
    return input == "true";
  };
  if (hasConfigValue("clusterizerMinTime")) {
    mClusterizerSettings.mMinCellTime = std::stod(getConfigValue("clusterizerMinTime"));
  }
  if (hasConfigValue("clusterizerMaxTime")) {
    mClusterizerSettings.mMaxCellTime = std::stod(getConfigValue("clusterizerMaxTime"));
  }
  if (hasConfigValue("clusterizerMaxTimeDelta")) {
    mClusterizerSettings.mMaxTimeDeltaCells = std::stod(getConfigValue("clusterizerMaxTimeDelta"));
  }
  if (hasConfigValue("clusterizerSeedThreshold")) {
    mClusterizerSettings.mSeedThreshold = std::stof(getConfigValue("clusterizerSeedThreshold"));
  }
  if (hasConfigValue("clusterizerCellTreshold")) {
    mClusterizerSettings.mCellThreshold = std::stof(getConfigValue("clusterizerCellTreshold"));
  }
  if (hasConfigValue("clusterizerDoGradientCut")) {
    mClusterizerSettings.mDoEnergyGradientCut = get_bool(getConfigValueLower("clusterizerDoGradientCut"));
  }
  if (hasConfigValue("clusterizerGradientCut")) {
    mClusterizerSettings.mGradientCut = std::stof(getConfigValue("clusterizerGradientCut"));
  }
}

void ClusterTask::resetHistograms()
{
  auto conditionalReset = [](auto obj) {
    if (obj) {
      obj->Reset();
    }
  };

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  conditionalReset(mHistNclustPerTF);
  conditionalReset(mHistNclustPerEvt);
  conditionalReset(mHistClustEtaPhi);

  conditionalReset(mHistTime_EMCal);
  conditionalReset(mHistClustE_EMCal);
  conditionalReset(mHistNCells_EMCal);
  conditionalReset(mHistM02_EMCal);
  conditionalReset(mHistM20_EMCal);
  conditionalReset(mHistM02VsClustE__EMCal);
  conditionalReset(mHistM20VsClustE__EMCal);

  conditionalReset(mHistClustE_DCal);
  conditionalReset(mHistNCells_DCal);
  conditionalReset(mHistM02_DCal);
  conditionalReset(mHistM20_DCal);
  conditionalReset(mHistM02VsClustE__DCal);
  conditionalReset(mHistM20VsClustE__DCal);
}

bool ClusterTask::hasConfigValue(const std::string_view key)
{
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    return true;
  }
  return false;
}

std::string ClusterTask::getConfigValue(const std::string_view key)
{
  std::string result;
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    result = param->second;
  }
  return result;
}

std::string ClusterTask::getConfigValueLower(const std::string_view key)
{
  auto input = getConfigValue(key);
  std::string result;
  if (input.length()) {
    result = boost::algorithm::to_lower_copy(input);
  }
  return result;
}

void ClusterTask::ClusterizerParams::print(std::ostream& stream) const
{
  stream << "Clusterizer Parameters: \n"
         << "=============================================\n"
         << "Min. time:                           " << mMinCellTime << " ns\n"
         << "Max. time:                           " << mMaxCellTime << " ns\n"
         << "Max. time difference between cells:  " << mMaxTimeDeltaCells << " ns\n"
         << "Seed threshold:                      " << mSeedThreshold << " GeV\n"
         << "Cell threshold:                      " << mCellThreshold << " GeV\n"
         << "Perform gradient cut:                " << (mDoEnergyGradientCut ? "yes" : "no") << "\n"
         << "Gradient cut:                        " << mGradientCut << "\n";
}

std::ostream& operator<<(std::ostream& stream, const ClusterTask::ClusterizerParams& params)
{
  params.print(stream);
  return stream;
}

} // namespace o2::quality_control_modules::emcal
