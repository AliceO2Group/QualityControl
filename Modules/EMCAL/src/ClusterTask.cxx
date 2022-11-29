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
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLorentzVector.h>

#include "EMCAL/ClusterTask.h"
#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

#include <gsl/span>

#include "CommonConstants/Triggers.h"
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

  conditionalDelete(mHistCellEnergyTimeUsed);
  conditionalDelete(mHistCellEnergyTimePhys);
  conditionalDelete(mHistCellEnergyTimeCalib);

  conditionalDelete(mHistNclustPerTF);
  conditionalDelete(mHistNclustPerEvt);
  conditionalDelete(mHistClustEtaPhi);
  conditionalDelete(mHistNclustPerTFSelected);
  conditionalDelete(mHistNclustPerEvtSelected);

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

  conditionalDelete(mHistMassDiphoton_EMCAL);
  conditionalDelete(mHistMassDiphoton_DCAL);
  conditionalDelete(mHistMassDiphotonPt_EMCAL);
  conditionalDelete(mHistMassDiphotonPt_DCAL);

  conditionalDelete(mHistNClustPerEvt_Calib);
  conditionalDelete(mHistNClustPerEvtSelected_Calib);
  conditionalDelete(mHistClusterEtaPhi_Calib);
  conditionalDelete(mHistClusterEnergy_Calib);
  conditionalDelete(mHistClusterEnergyTime_Calib);
  conditionalDelete(mHistClusterEnergyCells_Calib);
}

void ClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::setDetector("EMC");                        // svk
  ILOG(Info, Support) << "initialize ClusterTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  auto get_bool = [](const std::string_view input) -> bool { // svk
    return input == "true";
  };

  configureBindings();

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

  if (hasConfigValue("fillControlHistograms")) {
    mFillControlHistograms = get_bool(getConfigValueLower("fillControlHistograms"));
    if (mFillControlHistograms) {
      ILOG(Info, Support) << "Filling cell-level control histograms" << ENDM;
    }
  }

  if (hasConfigValue("hasInvMassMesons")) {
    mFillInvMassMeson = get_bool(getConfigValueLower("hasInvMassMesons"));
  }
  if (mFillInvMassMeson) {
    ILOG(Info, Support) << "Invariant mass histograms for Meson candidates enabled" << ENDM;
    configureMesonSelection();
  } else {
    ILOG(Info, Support) << "Invariant mass histograms for Meson candidates disabled" << ENDM;
  }

  mEventHandler = std::make_unique<o2::emcal::EventHandler<o2::emcal::Cell>>();
  mClusterFactory = std::make_unique<o2::emcal::ClusterFactory<o2::emcal::Cell>>();

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000); // svk
  mClusterFactory->setGeometry(mGeometry);

  mHistNclustPerTF = new TH1F("NclustPerTF", "Number of clusters per time frame;N_{Cluster}/TF;", 2000, 0.0, 200000.0); // svk
  getObjectsManager()->startPublishing(mHistNclustPerTF);

  mHistNclustPerEvt = new TH1F("NclustPerEvt", "Number of clusters per event;N_{Cluster}/Event;", 200, 0.0, 200.0); // svk
  getObjectsManager()->startPublishing(mHistNclustPerEvt);

  mHistClustEtaPhi = new TH2F("ClustEtaPhi", "Cluster #eta and #phi distribution;#eta;#phi", 100, -1.0, 1.0, 100, 0.0, 2 * TMath::Pi()); // svk
  getObjectsManager()->startPublishing(mHistClustEtaPhi);

  mHistNclustPerTFSelected = new TH1F("NclustPerTFSel", "Number of selected clusters per time frame;N_{Cluster}/TF;", 2000, 0.0, 200000.0);
  getObjectsManager()->startPublishing(mHistNclustPerTFSelected);

  mHistNclustPerEvtSelected = new TH1F("NclustPerEvtSel", "Number of clusters per event;N_{Cluster}/Event;", 200, 0.0, 200.0);
  getObjectsManager()->startPublishing(mHistNclustPerEvtSelected);

  ///////////////////
  // Control histograms (optional)
  ///////////////////
  if (mFillControlHistograms) {
    mHistCellEnergyTimeUsed = new TH2D("CellEnergyTimeUsedAll", "Cell energy vs time (all cells for clustering); E_{cell} (GeV); t_{cell} (ns)", 500, 0, 50, 1800, -900, 900);
    getObjectsManager()->startPublishing(mHistCellEnergyTimeUsed);
    mHistCellEnergyTimePhys = new TH2D("CellEnergyTimeUsedPhys", "Cell energy vs time (all cells for clustering, phys events); E_{cell} (GeV); t_{cell} (ns)", 500, 0, 50, 1800, -900, 900);
    getObjectsManager()->startPublishing(mHistCellEnergyTimePhys);
    mHistCellEnergyTimeCalib = new TH2D("CellEnergyTimeUsedCalib", "Cell energy vs time (all cells for clustering, calib events); E_{cell} (GeV); t_{cell} (ns)", 500, 0, 50, 1800, -900, 900);
    getObjectsManager()->startPublishing(mHistCellEnergyTimeCalib);
  }

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

  ///////////////////
  // Calib trigger histograms//
  ///////////////////

  mHistNClustPerEvt_Calib = new TH1F("NclustPerEvt_Calib", "Number of clusters per calib event;N_{Cluster}/Event;", 4000, 0.0, 4000.0);
  getObjectsManager()->startPublishing(mHistNClustPerEvt_Calib);

  mHistNClustPerEvtSelected_Calib = new TH1F("NclustPerEvtSel_Calib", "Number of selected clusters per calib event;N_{Cluster}/Event;", 4000, 0.0, 4000.0);
  getObjectsManager()->startPublishing(mHistNClustPerEvtSelected_Calib);

  mHistClusterEtaPhi_Calib = new TH2D("ClustEtaPhi_LED", "Cluster #eta-#phi position in LED events; #eta; #phi", 100, -1.0, 1.0, 100, 0.0, 2 * TMath::Pi());
  getObjectsManager()->startPublishing(mHistClusterEtaPhi_Calib);

  mHistClusterEnergy_Calib = new TH1D("ClusterEnergy_LED", "Cluster energy in LED events; E_{cl} (GeV); dN/dE_{cl} (GeV^{-1})", 200, 0., 20.);
  getObjectsManager()->startPublishing(mHistClusterEnergy_Calib);

  mHistClusterEnergyTime_Calib = new TH2D("ClusterEnergyTime_LED", "Cluster energy vs. time in LED events; E_{cl} (GeV); t (ns)", 500, 0, 50, 1800, -900, 900);
  getObjectsManager()->startPublishing(mHistClusterEnergyTime_Calib);

  mHistClusterEnergyCells_Calib = new TH2D("CusterEnergyNcell_LED", "Cluster energy vs. ncell in LED events; E_{cl} (GeV); n_{cell}", 500, 0, 50, 100, 0, 100);
  getObjectsManager()->startPublishing(mHistClusterEnergyCells_Calib);

  ///////////////////
  // Meson histograms//
  ///////////////////

  if (mFillInvMassMeson) {
    mHistMassDiphoton_EMCAL = new TH1D("InvMassDiphoton_EMCAL", "Diphoton invariant mass for pairs in EMCAL; m_{#gamma#gamma} (GeV/c^{2}); Number of candidates", 400, 0., 0.8);
    getObjectsManager()->startPublishing(mHistMassDiphoton_EMCAL);

    mHistMassDiphoton_DCAL = new TH1F("InvMassDiphoton_DCAL", "Diphoton invariant mass for pairs in DCAL; m_{#gamma#gamma} (GeV/c^{2}); Number of candidates", 400, 0., 0.8);
    getObjectsManager()->startPublishing(mHistMassDiphoton_DCAL);

    mHistMassDiphotonPt_EMCAL = new TH2D("InvMassDiphotonPt_EMCAL", "Diphoton invariant mass vs p_{t} for paris in EMCAL;  m_{#gamma#gamma} (GeV/c^{2}); p_{t,#gamma#gamma} (GeV/c)", 400, 0., 0.8, 200, 0., 20.);
    getObjectsManager()->startPublishing(mHistMassDiphotonPt_EMCAL);

    mHistMassDiphotonPt_DCAL = new TH2D("InvMassDiphotonPt_DCAL", "Diphoton invariant mass vs p_{t} for paris in DCAL;  m_{#gamma#gamma} (GeV/c^{2}); p_{t,#gamma#gamma} (GeV/c)", 400, 0., 0.8, 200, 0., 20.);
    getObjectsManager()->startPublishing(mHistMassDiphotonPt_DCAL);
  }
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
  auto cell = ctx.inputs().get<gsl::span<o2::emcal::Cell>>(mTaskInputBindings.mCellBinding.data());
  auto cellTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(mTaskInputBindings.mCellTriggerRecordBinding.data());

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
    auto cluster = ctx.inputs().get<gsl::span<o2::emcal::Cluster>>(mTaskInputBindings.mClusterBinding.data());
    auto clusterTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(mTaskInputBindings.mClusterTriggerRecordBinding.data());
    auto cellIndex = ctx.inputs().get<gsl::span<int>>(mTaskInputBindings.mCellIndexBinding.data());
    auto cellIndexTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(mTaskInputBindings.mCellIndexTriggerRecordBinding.data());

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
  // mClusterFactory->reset();

  mEventHandler->setClusterData(clusters, clusterIndices, clusterTriggerRecords, cellIndexTriggerRecords);
  mEventHandler->setCellData(cells, cellTriggerRecords);

  mHistNclustPerTF->Fill(clusters.size());
  int nClustersTimeframeSelected = 0;

  for (int iev = 0; iev < mEventHandler->getNumberOfEvents(); iev++) {
    auto inputEvent = mEventHandler->buildEvent(iev);
    auto trigger = inputEvent.mTriggerBits;
    auto isCalibTrigger = (trigger & o2::trigger::Cal),
         isPhysicsTrigger = (trigger & o2::trigger::PhT);
    if (isCalibTrigger) {
      mHistNClustPerEvt_Calib->Fill(inputEvent.mClusters.size());
    }
    if (isPhysicsTrigger) {
      mHistNclustPerEvt->Fill(inputEvent.mClusters.size());
    }

    mClusterFactory->reset();
    mClusterFactory->setClustersContainer(inputEvent.mClusters);
    mClusterFactory->setCellsContainer(inputEvent.mCells);
    mClusterFactory->setCellsIndicesContainer(inputEvent.mCellIndices);

    std::vector<TLorentzVector> selclustersEMCAL, selclustersDCAL;
    int nClustersEventSelected = 0;
    for (int icl = 0; icl < mClusterFactory->getNumberOfClusters(); icl++) {

      o2::emcal::AnalysisCluster analysisCluster = mClusterFactory->buildCluster(icl);
      if (isPhysicsTrigger) {
        if (analysisCluster.getIsExotic()) {
          continue;
        }
        auto clsTypeEMC = fillClusterHistogramsPhysics(analysisCluster);
        if (mFillInvMassMeson && mMesonClusterCuts.isSelected(analysisCluster)) {
          auto clustervec = buildClusterVector(analysisCluster);
          if (clsTypeEMC) {
            selclustersEMCAL.push_back(clustervec);
          } else {
            selclustersDCAL.push_back(clustervec);
          }
        }
        nClustersEventSelected++;
        nClustersTimeframeSelected++;
      }
      if (isCalibTrigger) {
        if (!analysisCluster.getIsExotic()) {
          fillClusterHistogramsLED(analysisCluster);
          nClustersEventSelected++;
          nClustersTimeframeSelected++;
        }
      }

    } // cls loop
    if (isPhysicsTrigger && mFillInvMassMeson) {
      buildAndAnalysePiOs(selclustersEMCAL, true);
      buildAndAnalysePiOs(selclustersDCAL, false);
    }
    if (isPhysicsTrigger) {
      mHistNclustPerEvtSelected->Fill(nClustersEventSelected);
    }
    if (isCalibTrigger) {
      mHistNClustPerEvtSelected_Calib->Fill(nClustersEventSelected);
    }
  } // event loop
  mHistNclustPerTFSelected->Fill(nClustersTimeframeSelected);
}

void ClusterTask::buildAndAnalysePiOs(const gsl::span<const TLorentzVector> clustervectors, bool isEMCAL)
{
  ILOG(Debug, Support) << "Next event: " << clustervectors.size() << " clusters from detector " << (isEMCAL ? "EMCAL" : "DCAL") << " for meson search" << ENDM;
  auto histMass1D = isEMCAL ? mHistMassDiphoton_EMCAL : mHistMassDiphoton_DCAL;
  auto histMassPt = isEMCAL ? mHistMassDiphotonPt_EMCAL : mHistMassDiphotonPt_DCAL;
  if (clustervectors.size() > 1) {
    for (int icl = 0; icl < clustervectors.size() - 1; icl++) {
      for (int jcl = icl + 1; jcl < clustervectors.size(); jcl++) {
        TLorentzVector pi0candidate = clustervectors[icl] + clustervectors[jcl];
        if (!mMesonCuts.isSelected(pi0candidate)) {
          continue;
        }
        histMassPt->Fill(pi0candidate.M(), pi0candidate.Pt());
        histMass1D->Fill(pi0candidate.M());
      }
    }
  }
}

TLorentzVector ClusterTask::buildClusterVector(const o2::emcal::AnalysisCluster& fullcluster) const
{
  auto pos = fullcluster.getGlobalPosition();
  auto theta = 2 * std::atan2(std::exp(-pos.Eta()), 1);
  auto px = fullcluster.E() * std::sin(theta) * std::cos(pos.Phi());
  auto py = fullcluster.E() * std::sin(theta) * std::sin(pos.Phi());
  auto pz = fullcluster.E() * std::cos(theta);
  TLorentzVector clustervec;
  clustervec.SetPxPyPzE(px, py, pz, fullcluster.E());
  return clustervec;
}

bool ClusterTask::fillClusterHistogramsPhysics(const o2::emcal::AnalysisCluster& cluster)
{
  Double_t clustE = cluster.E();
  Double_t clustT = cluster.getClusterTime(); // svk converted to ns adapted from Run2;

  /////////////////////////////////
  // Select EMCAL or DCAL clusters//
  /////////////////////////////////
  Bool_t clsTypeEMC;

  math_utils::Point3D<float> emcx = cluster.getGlobalPosition();

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
    mHistNCells_EMCal->Fill(clustE, cluster.getNCells());
    mHistM02_EMCal->Fill(cluster.getM02());
    mHistM20_EMCal->Fill(cluster.getM20());
    mHistM02VsClustE__EMCal->Fill(clustE, cluster.getM02()); // svk
    mHistM20VsClustE__EMCal->Fill(clustE, cluster.getM20()); // svk
  } else {
    mHistTime_DCal->Fill(clustE, clustT); // svk
    mHistClustE_DCal->Fill(clustE);
    mHistNCells_DCal->Fill(clustE, cluster.getNCells());
    mHistM02_DCal->Fill(cluster.getM02());
    mHistM20_DCal->Fill(cluster.getM20());
    mHistM02VsClustE__DCal->Fill(clustE, cluster.getM02()); // svk
    mHistM20VsClustE__DCal->Fill(clustE, cluster.getM20()); // svk
  }
  return clsTypeEMC;
}

void ClusterTask::fillClusterHistogramsLED(const o2::emcal::AnalysisCluster& cluster)
{
  math_utils::Point3D<float> emcx = cluster.getGlobalPosition();

  TVector3 clustpos(emcx.X(), emcx.Y(), emcx.Z());
  Double_t emcphi = TVector2::Phi_0_2pi(clustpos.Phi());
  Double_t emceta = clustpos.Eta();
  mHistClusterEnergy_Calib->Fill(cluster.E());
  mHistClusterEtaPhi_Calib->Fill(emceta, emcphi);
  mHistClusterEnergyTime_Calib->Fill(cluster.E(), cluster.getClusterTime());
  mHistClusterEnergyCells_Calib->Fill(cluster.E(), cluster.getNCells());
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
      auto cellsEvent = cells.subspan(iTrgRcrd.getFirstEntry(), iTrgRcrd.getNumberOfObjects());
      if (mFillControlHistograms) {
        auto isCalibTrigger = (iTrgRcrd.getTriggerBits() & o2::trigger::Cal),
             isPhysicsTrigger = (iTrgRcrd.getTriggerBits() & o2::trigger::PhT);
        for (auto& cell : cellsEvent) {
          mHistCellEnergyTimeUsed->Fill(cell.getAmplitude(), cell.getTimeStamp());
          if (isPhysicsTrigger) {
            mHistCellEnergyTimePhys->Fill(cell.getAmplitude(), cell.getTimeStamp());
          }
          if (isCalibTrigger) {
            mHistCellEnergyTimeCalib->Fill(cell.getAmplitude(), cell.getTimeStamp());
          }
        }
      }
      mClusterizer->findClusters(cellsEvent); // Find clusters on cells/digits (pass by ref)
    }

    auto outputClustersTemp = mClusterizer->getFoundClusters();
    LOG(debug) << " Number of Clusters in outputClustersTemp " << outputClustersTemp->size() << ENDM;

    auto outputCellDigitIndicesTemp = mClusterizer->getFoundClustersInputIndices();

    std::copy(outputClustersTemp->begin(), outputClustersTemp->end(), std::back_inserter(clusters));
    std::copy(outputCellDigitIndicesTemp->begin(), outputCellDigitIndicesTemp->end(), std::back_inserter(clusterIndices));

    clusterTriggerRecords.emplace_back(iTrgRcrd.getBCData(), currentStartClusters, outputClustersTemp->size()).setTriggerBits(iTrgRcrd.getTriggerBits());             // svk
    clusterIndexTriggerRecords.emplace_back(iTrgRcrd.getBCData(), currentStartIndices, outputCellDigitIndicesTemp->size()).setTriggerBits(iTrgRcrd.getTriggerBits()); // svk

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

void ClusterTask::configureBindings()
{
  if (hasConfigValue("bindingCells")) {
    mTaskInputBindings.mCellBinding = getConfigValue("bindingCells");
  }
  if (hasConfigValue("bindingCellTriggerRecords")) {
    mTaskInputBindings.mCellTriggerRecordBinding = getConfigValue("bindingCellTriggerRecords");
  }
  if (hasConfigValue("bindingClusters")) {
    mTaskInputBindings.mClusterBinding = getConfigValue("bindingClusters");
  }
  if (hasConfigValue("bindingClusterTriggerRecords")) {
    mTaskInputBindings.mClusterTriggerRecordBinding = getConfigValue("bindingClusterTriggerRecords");
  }
  if (hasConfigValue("bindingCellIndices")) {
    mTaskInputBindings.mCellIndexBinding = getConfigValue("bindingCellIndices");
  }
  if (hasConfigValue("bindingCellIndexTriggerRecords")) {
    mTaskInputBindings.mCellIndexTriggerRecordBinding = getConfigValue("bindingCellIndexTriggerRecords");
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

void ClusterTask::configureMesonSelection()
{
  auto get_bool = [](const std::string_view input) -> bool {
    return input == "true";
  };
  if (hasConfigValue("mesonClusterMinE")) {
    mMesonClusterCuts.mMinE = std::stod(getConfigValue("mesonClusterMinE"));
  }
  if (hasConfigValue("mesonClusterMaxTime")) {
    mMesonClusterCuts.mMaxTime = std::stod(getConfigValue("mesonClusterMaxTime"));
  }
  if (hasConfigValue("mesonClusterMinCells")) {
    mMesonClusterCuts.mMinNCell = std::stoi(getConfigValue("mesonClusterMinCells"));
  }
  if (hasConfigValue("mesonClustersRejectExotics")) {
    mMesonClusterCuts.mRejectExotics = std::stoi(getConfigValue("mesonClustersRejectExotics"));
  }
  if (hasConfigValue("mesonMinPt")) {
    mMesonCuts.mMinPt = std::stod(getConfigValue("mesonMinPt"));
  }

  ILOG(Info, Support) << mMesonClusterCuts;
  ILOG(Info, Support) << mMesonCuts;
}

void ClusterTask::resetHistograms()
{
  auto conditionalReset = [](auto obj) {
    if (obj) {
      obj->Reset();
    }
  };

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  conditionalReset(mHistCellEnergyTimeUsed);
  conditionalReset(mHistCellEnergyTimePhys);
  conditionalReset(mHistCellEnergyTimeCalib);
  conditionalReset(mHistNclustPerTFSelected);
  conditionalReset(mHistNclustPerEvtSelected);

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

  conditionalReset(mHistMassDiphoton_EMCAL);
  conditionalReset(mHistMassDiphoton_DCAL);
  conditionalReset(mHistMassDiphotonPt_EMCAL);
  conditionalReset(mHistMassDiphotonPt_DCAL);

  conditionalReset(mHistNClustPerEvt_Calib);
  conditionalReset(mHistNClustPerEvtSelected_Calib);
  conditionalReset(mHistClusterEtaPhi_Calib);
  conditionalReset(mHistClusterEnergy_Calib);
  conditionalReset(mHistClusterEnergyTime_Calib);
  conditionalReset(mHistClusterEnergyCells_Calib);
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

bool ClusterTask::MesonClusterSelection::isSelected(const o2::emcal::AnalysisCluster& cluster) const
{
  if (cluster.E() < mMinE) {
    return false;
  }
  if (std::abs(cluster.getClusterTime()) > mMaxTime) {
    return false;
  }
  if (cluster.getNCells() < mMinNCell) {
    return false;
  }
  if (mRejectExotics && cluster.getIsExotic()) {
    return false;
  }
  return true;
}

void ClusterTask::MesonClusterSelection::print(std::ostream& stream) const
{
  stream << "Cluster selection for meson candidates: \n"
         << "======================================================\n"
         << "Min. energy:          " << mMinE << " GeV\n"
         << "Max. time:            " << mMaxTime << " ns\n"
         << "Min. number of cells: " << mMinNCell << "\n"
         << "Reject exotics:       " << (mRejectExotics ? "yes" : "no") << "\n";
}

bool ClusterTask::MesonSelection::isSelected(const TLorentzVector& mesonCandidate) const
{
  if (mesonCandidate.Pt() < mMinPt) {
    return false;
  }
  return true;
}

void ClusterTask::MesonSelection::print(std::ostream& stream) const
{
  stream << "Meson candidates selection: \n"
         << "======================================================\n"
         << "Min. pt:   " << mMinPt << " GeV/c\n";
}

std::ostream& operator<<(std::ostream& stream, const ClusterTask::ClusterizerParams& params)
{
  params.print(stream);
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ClusterTask::MesonClusterSelection& cuts)
{
  cuts.print(stream);
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ClusterTask::MesonSelection& cuts)
{
  cuts.print(stream);
  return stream;
}

} // namespace o2::quality_control_modules::emcal
