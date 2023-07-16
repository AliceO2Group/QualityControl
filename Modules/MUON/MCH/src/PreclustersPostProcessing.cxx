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
/// \file    PreclustersPostProcessing.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH pre-clusters
/// \since   21/06/2022
///

#include "MCH/PreclustersPostProcessing.h"
#include "QualityControl/QcInfoLogger.h"
#include <TDatime.h>

using namespace o2::quality_control_modules::muonchambers;

void PreclustersPostProcessing::configure(const boost::property_tree::ptree& config)
{
  PostProcessingConfigMCH mchConfig(getID(), config);

  mRefTimeStamp = mchConfig.getParameter<int64_t>("ReferenceTimeStamp", -1);
  auto refDate = mchConfig.getParameter<std::string>("ReferenceDate");
  if (!refDate.empty()) {
    TDatime date(refDate.c_str());
    mRefTimeStamp = date.Convert();
  }
  ILOG(Info, Devel) << "Reference time stamp: " << mRefTimeStamp
                    << "  (" << refDate << ")" << AliceO2::InfoLogger::InfoLogger::endm;

  mFullHistos = mchConfig.getParameter<bool>("FullHistos", false);

  mCcdbObjects.emplace(effSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(clusterChargeSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(clusterSizeSourceName(), CcdbObjectHelper());

  if (mRefTimeStamp > 0) {
    mCcdbObjectsRef.emplace(effSourceName(), CcdbObjectHelper());
    mCcdbObjectsRef.emplace(clusterChargeSourceName(), CcdbObjectHelper());
    mCcdbObjectsRef.emplace(clusterSizeSourceName(), CcdbObjectHelper());
  }

  for (auto source : mchConfig.dataSources) {
    std::string sourceType, sourceName;
    splitDataSourceName(source.name, sourceType, sourceName);
    if (sourceType.empty()) {
      continue;
    }

    auto obj = mCcdbObjects.find(sourceType);
    if (obj != mCcdbObjects.end()) {
      obj->second.mPath = source.path;
      obj->second.mName = sourceName;
    }

    auto objRef = mCcdbObjectsRef.find(sourceType);
    if (objRef != mCcdbObjectsRef.end()) {
      objRef->second.mPath = source.path;
      objRef->second.mName = sourceName;
    }
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::createEfficiencyHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(effSourceName());
  if (obj != mCcdbObjects.end()) {
    mElecMapOnCycle = std::make_unique<HistoOnCycle<MergeableTH2Ratio>>();
  }

  //----------------------------------
  // Reference mean efficiency histogram
  //----------------------------------

  TH2F* hElecHistoRef{ nullptr };
  obj = mCcdbObjectsRef.find(effSourceName());
  if (obj != mCcdbObjectsRef.end() && obj->second.update(qcdb, mRefTimeStamp)) {
    ILOG(Info, Devel) << "Loaded reference plot \"" << obj->second.mObject->getName() << "\", time stamp " << mRefTimeStamp
                      << AliceO2::InfoLogger::InfoLogger::endm;
    hElecHistoRef = obj->second.get<TH2F>();
  }

  //----------------------------------
  // Efficiency plotters
  //----------------------------------

  mEfficiencyPlotter = std::make_unique<EfficiencyPlotter>("Efficiency/", hElecHistoRef, mFullHistos);
  mEfficiencyPlotter->publish(getObjectsManager());

  mEfficiencyPlotterOnCycle = std::make_unique<EfficiencyPlotter>("Efficiency/LastCycle/", hElecHistoRef, mFullHistos);
  mEfficiencyPlotterOnCycle->publish(getObjectsManager());

  //----------------------------------
  // Efficiency trends
  //----------------------------------

  mEfficiencyTrendsPlotter = std::make_unique<EfficiencyTrendsPlotter>("Trends/", hElecHistoRef, mFullHistos);
  mEfficiencyTrendsPlotter->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::createClusterChargeHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(clusterChargeSourceName());
  if (obj != mCcdbObjects.end()) {
    mClusterChargeOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
  }

  //----------------------------------
  // Reference mean cluster charge histogram
  //----------------------------------

  TH2F* histoRef{ nullptr };
  obj = mCcdbObjectsRef.find(clusterChargeSourceName());
  if (obj != mCcdbObjectsRef.end() && obj->second.update(qcdb, mRefTimeStamp)) {
    histoRef = obj->second.get<TH2F>();
  }

  //----------------------------------
  // Cluster charge plotters
  //----------------------------------

  mClusterChargePlotter = std::make_unique<ClusterChargePlotter>("ClusterCharge/", histoRef, mFullHistos);
  mClusterChargePlotter->publish(getObjectsManager());

  mClusterChargePlotterOnCycle = std::make_unique<ClusterChargePlotter>("ClusterCharge/LastCycle/", histoRef, mFullHistos);
  mClusterChargePlotterOnCycle->publish(getObjectsManager());

  //----------------------------------
  // Cluster charge trends
  //----------------------------------

  mClusterChargeTrendsPlotter = std::make_unique<ClusterChargeTrendsPlotter>("Trends/", histoRef, mFullHistos);
  mClusterChargeTrendsPlotter->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::createClusterSizeHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(clusterSizeSourceName());
  if (obj != mCcdbObjects.end()) {
    mClusterSizeOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
  }

  //----------------------------------
  // Reference mean cluster size histogram
  //----------------------------------

  TH2F* histoRef{ nullptr };
  obj = mCcdbObjectsRef.find(clusterSizeSourceName());
  if (obj != mCcdbObjectsRef.end() && obj->second.update(qcdb, mRefTimeStamp)) {
    histoRef = obj->second.get<TH2F>();
  }

  //----------------------------------
  // Cluster size plotters
  //----------------------------------

  mClusterSizePlotter = std::make_unique<ClusterSizePlotter>("ClusterSize/", histoRef, mFullHistos);
  mClusterSizePlotter->publish(getObjectsManager());

  mClusterSizePlotterOnCycle = std::make_unique<ClusterSizePlotter>("ClusterSize/LastCycle/", histoRef, mFullHistos);
  mClusterSizePlotterOnCycle->publish(getObjectsManager());

  //----------------------------------
  // Cluster size trends
  //----------------------------------

  mClusterSizeTrendsPlotter = std::make_unique<ClusterSizeTrendsPlotter>("Trends/", histoRef, mFullHistos);
  mClusterSizeTrendsPlotter->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  createEfficiencyHistos(t, &qcdb);
  createClusterChargeHistos(t, &qcdb);
  createClusterSizeHistos(t, &qcdb);

  //--------------------------------------------------
  // Detector quality histogram
  //--------------------------------------------------

  mHistogramQualityPerDE = std::make_unique<TH2F>("QualityFlagPerDE", "Quality Flag vs DE", getNumDE(), 0, getNumDE(), 3, 0, 3);
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(1, "Bad");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(2, "Medium");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(3, "Good");
  mHistogramQualityPerDE->SetOption("colz");
  mHistogramQualityPerDE->SetStats(0);
  getObjectsManager()->startPublishing(mHistogramQualityPerDE.get());
  getObjectsManager()->setDefaultDrawOptions(mHistogramQualityPerDE.get(), "colz");
  getObjectsManager()->setDisplayHint(mHistogramQualityPerDE.get(), "gridy");
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::updateEfficiencyHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(effSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    MergeableTH2Ratio* hr = obj->second.get<MergeableTH2Ratio>();
    if (hr) {
      mEfficiencyPlotter->update(hr);
      // extract the average occupancies on the last cycle
      mElecMapOnCycle->update(hr);
      mEfficiencyPlotterOnCycle->update(mElecMapOnCycle.get());

      auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
      mEfficiencyTrendsPlotter->update(time, mElecMapOnCycle.get());
    }
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::updateClusterChargeHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(clusterChargeSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2F* h = obj->second.get<TH2F>();
    if (h) {
      mClusterChargePlotter->update(h);
      // extract the average occupancies on the last cycle
      mClusterChargeOnCycle->update(h);
      mClusterChargePlotterOnCycle->update(mClusterChargeOnCycle.get());

      auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
      mClusterChargeTrendsPlotter->update(time, mClusterChargeOnCycle.get());
    }
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::updateClusterSizeHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(clusterSizeSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2F* h = obj->second.get<TH2F>();
    if (h) {
      mClusterSizePlotter->update(h);
      // extract the average occupancies on the last cycle
      mClusterSizeOnCycle->update(h);
      mClusterSizePlotterOnCycle->update(mClusterSizeOnCycle.get());

      auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
      mClusterSizeTrendsPlotter->update(time, mClusterSizeOnCycle.get());
    }
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateEfficiencyHistos(t, &qcdb);
  updateClusterChargeHistos(t, &qcdb);
  updateClusterSizeHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
