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
#include "MUONCommon/Helpers.h"
#include "Common/ReferenceComparatorPlot.h"
#include "QualityControl/ReferenceUtils.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2::quality_control_modules::muonchambers;
using namespace o2::quality_control_modules::muon;

void PreclustersPostProcessing::configure(const boost::property_tree::ptree& config)
{
  ReferenceComparatorTask::configure(config);

  mConfig = PostProcessingConfigMCH(getID(), config);
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::createEfficiencyHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //----------------------------------
  // Efficiency plotters
  //----------------------------------

  mEfficiencyPlotter.reset();
  mEfficiencyPlotter = std::make_unique<EfficiencyPlotter>("Efficiency/", mFullHistos);
  mEfficiencyPlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

  for (auto& hinfo : mEfficiencyPlotter->histograms()) {
    TH1* hist = dynamic_cast<TH1*>(hinfo.object);
    if (hist) {
      mHistogramsAll.push_back(hist);
    }
  }

  if (mEnableLastCycleHistos) {
    // Helpers to extract plots from last cycle
    auto obj = mCcdbObjects.find(effSourceName());
    if (obj != mCcdbObjects.end()) {
      mElecMapOnCycle = std::make_unique<HistoOnCycle<TH2FRatio>>();
    }

    mEfficiencyPlotterOnCycle.reset();
    mEfficiencyPlotterOnCycle = std::make_unique<EfficiencyPlotter>("Efficiency/LastCycle/", mFullHistos);
    mEfficiencyPlotterOnCycle->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

    for (auto& hinfo : mEfficiencyPlotterOnCycle->histograms()) {
      TH1* hist = dynamic_cast<TH1*>(hinfo.object);
      if (hist) {
        mHistogramsAll.push_back(hist);
      }
    }
  }

  //----------------------------------
  // Efficiency trends
  //----------------------------------

  if (mEnableTrending) {
    mEfficiencyTrendsPlotter.reset();
    mEfficiencyTrendsPlotter = std::make_unique<EfficiencyTrendsPlotter>("Trends/", mFullHistos);
    mEfficiencyTrendsPlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::createClusterChargeHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //----------------------------------
  // Cluster charge plotters
  //----------------------------------

  mClusterChargePlotter.reset();
  mClusterChargePlotter = std::make_unique<ClusterChargePlotter>("ClusterCharge/", mFullHistos);
  mClusterChargePlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

  if (mEnableLastCycleHistos) {
    // Helpers to extract plots from last cycle
    auto obj = mCcdbObjects.find(clusterChargeSourceName());
    if (obj != mCcdbObjects.end()) {
      mClusterChargeOnCycle.reset();
      mClusterChargeOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
    }

    mClusterChargePlotterOnCycle.reset();
    mClusterChargePlotterOnCycle = std::make_unique<ClusterChargePlotter>("ClusterCharge/LastCycle/", mFullHistos);
    mClusterChargePlotterOnCycle->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);
  }

  //----------------------------------
  // Cluster charge trends
  //----------------------------------

  if (mEnableTrending) {
    mClusterChargeTrendsPlotter.reset();
    mClusterChargeTrendsPlotter = std::make_unique<ClusterChargeTrendsPlotter>("Trends/", mFullHistos);
    mClusterChargeTrendsPlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::createClusterSizeHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //----------------------------------
  // Cluster size plotters
  //----------------------------------

  mClusterSizePlotter.reset();
  mClusterSizePlotter = std::make_unique<ClusterSizePlotter>("ClusterSize/", mFullHistos);
  mClusterSizePlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

  if (mEnableLastCycleHistos) {
    // Helpers to extract plots from last cycle
    auto obj = mCcdbObjects.find(clusterSizeSourceName());
    if (obj != mCcdbObjects.end()) {
      mClusterSizeOnCycle.reset();
      mClusterSizeOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
    }

    mClusterSizePlotterOnCycle.reset();
    mClusterSizePlotterOnCycle = std::make_unique<ClusterSizePlotter>("ClusterSize/LastCycle/", mFullHistos);
    mClusterSizePlotterOnCycle->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);
  }

  //----------------------------------
  // Cluster size trends
  //----------------------------------

  if (mEnableTrending) {
    mClusterSizeTrendsPlotter.reset();
    mClusterSizeTrendsPlotter = std::make_unique<ClusterSizeTrendsPlotter>("Trends/", mFullHistos);
    mClusterSizeTrendsPlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  ReferenceComparatorTask::initialize(t, services);

  auto& qcdb = services.get<repository::DatabaseInterface>();
  const auto& activity = t.activity;

  mFullHistos = getConfigurationParameter<bool>(mCustomParameters, "FullHistos", mFullHistos, activity);
  mEnableLastCycleHistos = getConfigurationParameter<bool>(mCustomParameters, "EnableLastCycleHistos", mEnableLastCycleHistos, activity);
  mEnableTrending = getConfigurationParameter<bool>(mCustomParameters, "EnableTrending", mEnableTrending, activity);

  mCcdbObjects.clear();
  mCcdbObjects.emplace(effSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(clusterChargeSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(clusterSizeSourceName(), CcdbObjectHelper());

  // set objects path from configuration
  for (const auto& source : mConfig.dataSources) {
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
  }

  // instantiate and publish the histograms
  createEfficiencyHistos(t, &qcdb);
  createClusterChargeHistos(t, &qcdb);
  createClusterSizeHistos(t, &qcdb);

  //--------------------------------------------------
  // Quality histogram
  //--------------------------------------------------

  mHistogramQualityPerDE.reset();
  mHistogramQualityPerDE = std::make_unique<TH2F>("QualityFlagPerDE", "Quality Flag vs DE", getNumDE(), 0, getNumDE(), 3, 0, 3);
  addDEBinLabels(mHistogramQualityPerDE.get());
  addChamberDelimiters(mHistogramQualityPerDE.get());
  addChamberLabelsForDE(mHistogramQualityPerDE.get());
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(1, "Bad");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(2, "Medium");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(3, "Good");
  mHistogramQualityPerDE->SetOption("colz");
  mHistogramQualityPerDE->SetStats(0);
  getObjectsManager()->startPublishing(mHistogramQualityPerDE.get(), core::PublicationPolicy::ThroughStop);
  getObjectsManager()->setDefaultDrawOptions(mHistogramQualityPerDE.get(), "colz");
  getObjectsManager()->setDisplayHint(mHistogramQualityPerDE.get(), "gridy");

  mHistogramQualityPerSolar.reset();
  mHistogramQualityPerSolar = std::make_unique<TH2F>("QualityFlagPerSolar", "Quality Flag vs Solar", getNumSolar(), 0, getNumSolar(), 3, 0, 3);
  addSolarBinLabels(mHistogramQualityPerSolar.get());
  addChamberDelimitersToSolarHistogram(mHistogramQualityPerSolar.get());
  addChamberLabelsForSolar(mHistogramQualityPerSolar.get());
  mHistogramQualityPerSolar->GetYaxis()->SetBinLabel(1, "Bad");
  mHistogramQualityPerSolar->GetYaxis()->SetBinLabel(2, "Medium");
  mHistogramQualityPerSolar->GetYaxis()->SetBinLabel(3, "Good");
  mHistogramQualityPerSolar->SetOption("col");
  mHistogramQualityPerSolar->SetStats(0);
  getObjectsManager()->startPublishing(mHistogramQualityPerSolar.get(), core::PublicationPolicy::ThroughStop);
  getObjectsManager()->setDefaultDrawOptions(mHistogramQualityPerSolar.get(), "col");
  getObjectsManager()->setDisplayHint(mHistogramQualityPerSolar.get(), "gridy");
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::updateEfficiencyHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(effSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2FRatio* hr = obj->second.get<TH2FRatio>();
    if (hr) {
      mEfficiencyPlotter->update(hr);
      if (mEnableLastCycleHistos) {
        // extract the average occupancies on the last cycle
        mElecMapOnCycle->update(hr);
        mEfficiencyPlotterOnCycle->update(mElecMapOnCycle.get());
      }

      if (mEnableTrending) {
        auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
        if (mEnableLastCycleHistos) {
          mEfficiencyTrendsPlotter->update(time, mElecMapOnCycle.get());
        } else {
          mEfficiencyTrendsPlotter->update(time, hr);
        }
      }
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
      if (mEnableLastCycleHistos) {
        // extract the average occupancies on the last cycle
        mClusterChargeOnCycle->update(h);
        mClusterChargePlotterOnCycle->update(mClusterChargeOnCycle.get());
      }

      if (mEnableTrending) {
        auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
        if (mEnableLastCycleHistos) {
          mClusterChargeTrendsPlotter->update(time, mClusterChargeOnCycle.get());
        } else {
          mClusterChargeTrendsPlotter->update(time, h);
        }
      }
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
      if (mEnableLastCycleHistos) {
        // extract the average occupancies on the last cycle
        mClusterSizeOnCycle->update(h);
        mClusterSizePlotterOnCycle->update(mClusterSizeOnCycle.get());
      }

      if (mEnableTrending) {
        auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
        if (mEnableLastCycleHistos) {
          mClusterSizeTrendsPlotter->update(time, mClusterSizeOnCycle.get());
        } else {
          mClusterSizeTrendsPlotter->update(time, h);
        }
      }
    }
  }
}

//_________________________________________________________________________________________

TH1* PreclustersPostProcessing::getHistogram(std::string plotName)
{
  TH1* result{ nullptr };
  for (auto hist : mHistogramsAll) {
    if (plotName == hist->GetName()) {
      result = hist;
      break;
    }
  }
  return result;
}

void PreclustersPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateEfficiencyHistos(t, &qcdb);
  updateClusterChargeHistos(t, &qcdb);
  updateClusterSizeHistos(t, &qcdb);

  auto& comparatorPlots = getComparatorPlots();
  for (auto& [plotName, plot] : comparatorPlots) {
    TH1* hist = getHistogram(plotName);
    if (!hist) {
      continue;
    }

    plot->update(hist);
  }
}

//_________________________________________________________________________________________

void PreclustersPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef services)
{
  ReferenceComparatorTask::finalize(t, services);
}
