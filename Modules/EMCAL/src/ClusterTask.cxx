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
#include "Framework/ControlService.h"
#include "Framework/Logger.h"
#include <TGeoManager.h>

#include <DataFormatsEMCAL/EventHandler.h>
#include <EMCALBase/ClusterFactory.h>

#include <DataFormatsEMCAL/AnalysisCluster.h>

#include <EMCALReconstruction/Clusterizer.h> //svk

std::unique_ptr<o2::emcal::EventHandler<o2::emcal::Cell>> mEventHandler;
std::unique_ptr<o2::emcal::ClusterFactory<o2::emcal::Cell>> mClusterFactory;
std::unique_ptr<o2::emcal::Clusterizer<o2::emcal::Cell>> mClusterizer; //svk

namespace o2::quality_control_modules::emcal
{

ClusterTask::~ClusterTask()
{
  delete mHistClustE_EMCal;
  delete mHistNCells_EMCal;
  delete mHistM02_EMCal;
  delete mHistM20_EMCal;

  delete mHistClustE_DCal;
  delete mHistNCells_DCal;
  delete mHistM02_DCal;
  delete mHistM20_DCal;
}

void ClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::setDetector("EMC");                        //svk
  ILOG(Info, Support) << "initialize ClusterTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  auto get_bool = [](const std::string_view input) -> bool { //svk
    return input == "true";
  };

  if (hasConfigValue("useInternalClusterizer")) {
    mInternalClusterizer = get_bool(getConfigValueLower("useInternalClusterizer"));
    if (mInternalClusterizer) {
      LOG(debug) << "Enabling internal clusterizer . . . " << ENDM;
    }
  }

  mEventHandler = std::make_unique<o2::emcal::EventHandler<o2::emcal::Cell>>();
  mClusterFactory = std::make_unique<o2::emcal::ClusterFactory<o2::emcal::Cell>>();

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000); //svk

  mHistClustE_EMCal = new TH1F("ClustE_EMCal", "Cluster energy distribution; Cluster E(GeV);Counts", 500, 0.0, 50.0);
  getObjectsManager()->startPublishing(mHistClustE_EMCal);

  mHistNCells_EMCal = new TH2F("Cells_EMCal", "No of cells in a cluster;Cluster E;N^{EMC}_{cells}", 500, 0, 50, 30, 0, 30);
  getObjectsManager()->startPublishing(mHistNCells_EMCal);

  mHistM02_EMCal = new TH1F("M02_EMCal", "M02 distribution; M02;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM02_EMCal);

  mHistM20_EMCal = new TH1F("M20_EMCal", "M20 distribution; M20;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM20_EMCal);

  mHistClustE_DCal = new TH1F("ClustE_DCal", "Cluster energy distribution; Cluster E(GeV);Counts", 500, 0.0, 50.0);
  getObjectsManager()->startPublishing(mHistClustE_DCal);

  mHistNCells_DCal = new TH2F("Cells_DCal", "No of cells in a cluster;Cluster E;N^{EMC}_{cells}", 500, 0, 50, 30, 0, 30);
  getObjectsManager()->startPublishing(mHistNCells_DCal);

  mHistM02_DCal = new TH1F("M02_DCal", "M02 distribution; M02;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM02_DCal);

  mHistM20_DCal = new TH1F("M20_DCal", "M20 distribution; M20;Counts", 200, 0.0, 2.0);
  getObjectsManager()->startPublishing(mHistM20_DCal);
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
    TaskInterface::retrieveCondition("GLO/Config/Geometry");
  }

  resetHistograms();
}

void ClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::string InputCell = "emcal-cells";
  std::string InputCellTR = "emcal-cellstriggerecords";

  auto Cell = ctx.inputs().get<gsl::span<o2::emcal::Cell>>(InputCell.c_str());
  auto CellTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(InputCellTR.c_str());

  if (mInternalClusterizer) {
    std::vector<o2::emcal::Cluster> Cluster;
    std::vector<int> CellIndex;
    std::vector<o2::emcal::TriggerRecord> ClusterTR, CellIndexTR;

    findClustersInternal(Cell, CellTR, Cluster, ClusterTR, CellIndex, CellIndexTR); //do internal clustering here
    ILOG(Info, Support) << "Found  " << Cluster.size() << " CLusters  " << ENDM;
    ILOG(Info, Support) << "Found  " << ClusterTR.size() << " ClusterTR  " << ENDM;
    ILOG(Info, Support) << "Found  " << CellIndex.size() << " CellIndex  " << ENDM;
    ILOG(Info, Support) << "Found  " << CellIndexTR.size() << " CellIndexTR  " << ENDM;

    analyseTimeframe(Cell, CellTR, Cluster, ClusterTR, CellIndex, CellIndexTR); // Fill histos
  } else {

    std::string InputCluster = "emcal-clusters";
    std::string InputClusterTR = "emcal-clustertriggerecords";

    std::string InputCellIndex = "emcal-cellindices";
    std::string InputCellIndexTR = "emcal-citriggerecords";

    auto Cluster = ctx.inputs().get<gsl::span<o2::emcal::Cluster>>(InputCluster.c_str());
    auto ClusterTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(InputClusterTR.c_str());
    auto CellIndex = ctx.inputs().get<gsl::span<int>>(InputCellIndex.c_str());
    auto CellIndexTR = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(InputCellIndexTR.c_str());

    analyseTimeframe(Cell, CellTR, Cluster, ClusterTR, CellIndex, CellIndexTR); // Fill histos
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
  mEventHandler = std::make_unique<o2::emcal::EventHandler<o2::emcal::Cell>>();
  mClusterFactory = std::make_unique<o2::emcal::ClusterFactory<o2::emcal::Cell>>();

  mEventHandler->setClusterData(clusters, clusterIndices, clusterTriggerRecords, cellIndexTriggerRecords);
  mEventHandler->setCellData(cells, cellTriggerRecords);

  for (int iev = 0; iev < mEventHandler->getNumberOfEvents(); iev++) {
    auto inputEvent = mEventHandler->buildEvent(iev);

    mClusterFactory->setClustersContainer(inputEvent.mClusters);
    mClusterFactory->setCellsContainer(inputEvent.mCells);
    mClusterFactory->setCellsIndicesContainer(inputEvent.mCellIndices);

    for (int icl = 0; icl < mClusterFactory->getNumberOfClusters(); icl++) {

      o2::emcal::AnalysisCluster analysisCluster = mClusterFactory->buildCluster(icl);

      Double_t clustE = analysisCluster.E();

      /////////////////////////////////
      // Select EMCAL or DCAL clusters//
      /////////////////////////////////
      Bool_t clsTypeEMC;

      math_utils::Point3D<float> emcx = analysisCluster.getGlobalPosition();

      TVector3 clustpos(emcx.X(), emcx.Y(), emcx.Z());
      Double_t emcphi = TVector2::Phi_0_2pi(clustpos.Phi());
      Double_t emceta = clustpos.Eta();

      if (emcphi < 4.)
        clsTypeEMC = kTRUE; // EMCAL : 80 < phi < 187
      else
        clsTypeEMC = kFALSE; // DCAL  : 260 < phi < 327

      if (clsTypeEMC) {
        mHistClustE_EMCal->Fill(clustE);
        mHistNCells_EMCal->Fill(clustE, analysisCluster.getNCells());
        mHistM02_EMCal->Fill(analysisCluster.getM02());
        mHistM20_EMCal->Fill(analysisCluster.getM20());
      } else {
        mHistClustE_DCal->Fill(clustE);
        mHistNCells_DCal->Fill(clustE, analysisCluster.getNCells());
        mHistM02_DCal->Fill(analysisCluster.getM02());
        mHistM20_DCal->Fill(analysisCluster.getM20());
      }

    } // cls loop

  } // event loop
}

//_____________________________  Internal clusteriser function _____________________________
void ClusterTask::findClustersInternal(const gsl::span<const o2::emcal::Cell>& cells, const gsl::span<const o2::emcal::TriggerRecord>& cellTriggerRecords, std::vector<o2::emcal::Cluster>& clusters, std::vector<o2::emcal::TriggerRecord>& clusterTriggerRecords, std::vector<int>& clusterIndices, std::vector<o2::emcal::TriggerRecord>& clusterIndexTriggerRecords)
{
  LOG(debug) << "[EMCALClusterizer - findClustersInternal] called";
  //mTimer.Start(false);

  ILOG(Info, Support) << " findClustersInternal Called  " << ENDM;

  if (!mClusterizer) {
    mClusterizer = std::make_unique<o2::emcal::Clusterizer<o2::emcal::Cell>>();
    ILOG(Info, Support) << " mClusterizer Initialized  " << ENDM;
  }

  // FIXME: Placeholder configuration -> get config from CCDB object
  double timeCut = 10000, timeMin = 0, timeMax = 10000, gradientCut = 0.03, thresholdSeedEnergy = 0.1, thresholdCellEnergy = 0.05; //svk
  bool doEnergyGradientCut = true;

  // Initialize clusterizer and link geometry
  mClusterizer->initialize(timeCut, timeMin, timeMax, gradientCut, doEnergyGradientCut, thresholdSeedEnergy, thresholdCellEnergy);
  mClusterizer->setGeometry(mGeometry);

  clusters.clear();
  clusterTriggerRecords.clear();
  clusterIndices.clear();
  clusterIndexTriggerRecords.clear();

  int currentStartClusters = 0; //clusters->size();  //svk
  int currentStartIndices = 0;  //clusterIndices->size();  //svk

  ILOG(Info, Support) << "  start clustering " << cellTriggerRecords.size() << ENDM;

  for (auto iTrgRcrd : cellTriggerRecords) {
    ILOG(Info, Support) << " findClustersInternal loop over iTrgRcrd  " << ENDM;

    mClusterizer->clear();
    if (cells.size() && iTrgRcrd.getNumberOfObjects()) {
      ILOG(Info, Support) << " Number of cells put in " << cells.size() << ENDM;

      mClusterizer->findClusters(gsl::span<const o2::emcal::Cell>(&cells[iTrgRcrd.getFirstEntry()], iTrgRcrd.getNumberOfObjects())); // Find clusters on cells/digits (pass by ref)
    }

    auto outputClustersTemp = mClusterizer->getFoundClusters();

    ILOG(Info, Support) << " Number of Clusters in outputClustersTemp " << outputClustersTemp->size() << ENDM;

    auto outputCellDigitIndicesTemp = mClusterizer->getFoundClustersInputIndices();

    std::copy(outputClustersTemp->begin(), outputClustersTemp->end(), std::back_inserter(clusters));
    std::copy(outputCellDigitIndicesTemp->begin(), outputCellDigitIndicesTemp->end(), std::back_inserter(clusterIndices));

    clusterTriggerRecords.emplace_back(iTrgRcrd.getBCData(), currentStartClusters, outputClustersTemp->size());             //svk
    clusterIndexTriggerRecords.emplace_back(iTrgRcrd.getBCData(), currentStartIndices, outputCellDigitIndicesTemp->size()); //svk

    currentStartClusters = clusters.size();      //svk
    currentStartIndices = clusterIndices.size(); //svk
  }
  LOG(debug) << "[EMCALClusterizer - findClustersInternal] Writing " << clusters.size() << " clusters ...";
  //mTimer.Stop();
}

void ClusterTask::resetHistograms()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mHistClustE_EMCal->Reset();
  mHistNCells_EMCal->Reset();
  mHistM02_EMCal->Reset();
  mHistM20_EMCal->Reset();

  mHistClustE_DCal->Reset();
  mHistNCells_DCal->Reset();
  mHistM02_DCal->Reset();
  mHistM20_DCal->Reset();
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

} // namespace o2::quality_control_modules::emcal
