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
/// \file    DigitsPostProcessing.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH digits
/// \since   21/06/2022
///

#include "MCH/DigitsPostProcessing.h"
#include "MUONCommon/Helpers.h"
#include "Common/ReferenceComparatorPlot.h"
#include "QualityControl/ReferenceUtils.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include <TDatime.h>

using namespace o2::quality_control_modules::muonchambers;
using namespace o2::quality_control_modules::muon;

void DigitsPostProcessing::configure(const boost::property_tree::ptree& config)
{
  ReferenceComparatorTask::configure(config);

  mConfig = PostProcessingConfigMCH(getID(), config);
}

//_________________________________________________________________________________________

void DigitsPostProcessing::createRatesHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //----------------------------------
  // Rate plotters
  //----------------------------------

  mRatesPlotter.reset();
  mRatesPlotter = std::make_unique<RatesPlotter>("Rates/", mChannelRateMin, mChannelRateMax, true, mFullHistos);
  mRatesPlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

  for (auto& hinfo : mRatesPlotter->histograms()) {
    TH1* hist = dynamic_cast<TH1*>(hinfo.object);
    if (hist) {
      mHistogramsAll.push_back(hist);
    }
  }

  mRatesPlotterSignal.reset();
  mRatesPlotterSignal = std::make_unique<RatesPlotter>("RatesSignal/", mChannelRateMin, mChannelRateMax, true, mFullHistos);
  mRatesPlotterSignal->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

  for (auto& hinfo : mRatesPlotterSignal->histograms()) {
    TH1* hist = dynamic_cast<TH1*>(hinfo.object);
    if (hist) {
      mHistogramsAll.push_back(hist);
    }
  }

  //----------------------------------
  // Rate plotters for last cycle
  //----------------------------------

  if (mEnableLastCycleHistos) {
    // Helpers to extract plots from last cycle
    auto obj = mCcdbObjects.find(rateSourceName());
    if (obj != mCcdbObjects.end()) {
      mElecMapOnCycle.reset();
      mElecMapOnCycle = std::make_unique<HistoOnCycle<TH2FRatio>>();
    }

    obj = mCcdbObjects.find(rateSignalSourceName());
    if (obj != mCcdbObjects.end()) {
      mElecMapSignalOnCycle.reset();
      mElecMapSignalOnCycle = std::make_unique<HistoOnCycle<TH2FRatio>>();
    }

    mRatesPlotterOnCycle.reset();
    mRatesPlotterOnCycle = std::make_unique<RatesPlotter>("Rates/LastCycle/", mChannelRateMin, mChannelRateMax, false, mFullHistos);
    mRatesPlotterOnCycle->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

    for (auto& hinfo : mRatesPlotterOnCycle->histograms()) {
      TH1* hist = dynamic_cast<TH1*>(hinfo.object);
      if (hist) {
        mHistogramsAll.push_back(hist);
      }
    }

    mRatesPlotterSignalOnCycle.reset();
    mRatesPlotterSignalOnCycle = std::make_unique<RatesPlotter>("RatesSignal/LastCycle/", mChannelRateMin, mChannelRateMax, false, mFullHistos);
    mRatesPlotterSignalOnCycle->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

    for (auto& hinfo : mRatesPlotterSignalOnCycle->histograms()) {
      TH1* hist = dynamic_cast<TH1*>(hinfo.object);
      if (hist) {
        mHistogramsAll.push_back(hist);
      }
    }
  }

  //----------------------------------
  // Rate trends
  //----------------------------------

  if (mEnableTrending) {
    mRatesTrendsPlotter.reset();
    mRatesTrendsPlotter = std::make_unique<RatesTrendsPlotter>("Trends/Rates/", mFullHistos);
    mRatesTrendsPlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

    mRatesTrendsPlotterSignal.reset();
    mRatesTrendsPlotterSignal = std::make_unique<RatesTrendsPlotter>("Trends/RatesSignal/", mFullHistos);
    mRatesTrendsPlotterSignal->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);
  }
}

//_________________________________________________________________________________________

void DigitsPostProcessing::createOrbitHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //----------------------------------
  // Orbit plotters
  //----------------------------------

  mOrbitsPlotter.reset();
  mOrbitsPlotter = std::make_unique<OrbitsPlotter>("Orbits/");
  mOrbitsPlotter->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

  for (auto& hinfo : mOrbitsPlotter->histograms()) {
    TH1* hist = dynamic_cast<TH1*>(hinfo.object);
    if (hist) {
      mHistogramsAll.push_back(hist);
    }
  }

  mOrbitsPlotterSignal.reset();
  mOrbitsPlotterSignal = std::make_unique<OrbitsPlotter>("OrbitsSignal/");
  mOrbitsPlotterSignal->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

  for (auto& hinfo : mOrbitsPlotterSignal->histograms()) {
    TH1* hist = dynamic_cast<TH1*>(hinfo.object);
    if (hist) {
      mHistogramsAll.push_back(hist);
    }
  }

  //----------------------------------
  // Orbit plotters for last cycle
  //----------------------------------

  if (mEnableLastCycleHistos) {
    // Helpers to extract plots from last cycle
    auto obj = mCcdbObjects.find(orbitsSourceName());
    if (obj != mCcdbObjects.end()) {
      mDigitsOrbitsOnCycle.reset();
      mDigitsOrbitsOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
    }

    obj = mCcdbObjects.find(orbitsSignalSourceName());
    if (obj != mCcdbObjects.end()) {
      mDigitsSignalOrbitsOnCycle.reset();
      mDigitsSignalOrbitsOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
    }

    mOrbitsPlotterOnCycle.reset();
    mOrbitsPlotterOnCycle = std::make_unique<OrbitsPlotter>("Orbits/LastCycle/");
    mOrbitsPlotterOnCycle->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

    for (auto& hinfo : mOrbitsPlotterOnCycle->histograms()) {
      TH1* hist = dynamic_cast<TH1*>(hinfo.object);
      if (hist) {
        mHistogramsAll.push_back(hist);
      }
    }

    mOrbitsPlotterSignalOnCycle.reset();
    mOrbitsPlotterSignalOnCycle = std::make_unique<OrbitsPlotter>("OrbitsSignal/LastCycle/");
    mOrbitsPlotterSignalOnCycle->publish(getObjectsManager(), core::PublicationPolicy::ThroughStop);

    for (auto& hinfo : mOrbitsPlotterSignalOnCycle->histograms()) {
      TH1* hist = dynamic_cast<TH1*>(hinfo.object);
      if (hist) {
        mHistogramsAll.push_back(hist);
      }
    }
  }
}

//_________________________________________________________________________________________

void DigitsPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  ReferenceComparatorTask::initialize(t, services);

  auto& qcdb = services.get<repository::DatabaseInterface>();
  const auto& activity = t.activity;

  mFullHistos = getConfigurationParameter<bool>(mCustomParameters, "FullHistos", mFullHistos, activity);
  mEnableLastCycleHistos = getConfigurationParameter<bool>(mCustomParameters, "EnableLastCycleHistos", mEnableLastCycleHistos, activity);
  mEnableTrending = getConfigurationParameter<bool>(mCustomParameters, "EnableTrending", mEnableTrending, activity);

  mChannelRateMin = getConfigurationParameter<float>(mCustomParameters, "ChannelRateMin", mChannelRateMin, activity);
  mChannelRateMax = getConfigurationParameter<float>(mCustomParameters, "ChannelRateMax", mChannelRateMax, activity);

  mCcdbObjects.clear();
  mCcdbObjects.emplace(rateSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(rateSignalSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(orbitsSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(orbitsSignalSourceName(), CcdbObjectHelper());

  // set objects path from configuration
  for (auto source : mConfig.dataSources) {
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
  createRatesHistos(t, &qcdb);
  createOrbitHistos(t, &qcdb);

  //--------------------------------------------------
  // Detector quality histogram
  //--------------------------------------------------

  mHistogramQualityPerDE.reset();
  mHistogramQualityPerDE = std::make_unique<TH2F>("QualityFlagPerDE", "Quality Flag vs DE", getNumDE(), 0, getNumDE(), 3, 0, 3);
  addDEBinLabels(mHistogramQualityPerDE.get());
  addChamberDelimiters(mHistogramQualityPerDE.get());
  addChamberLabelsForDE(mHistogramQualityPerDE.get());
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(1, "Bad");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(2, "Medium");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(3, "Good");
  mHistogramQualityPerDE->SetOption("col");
  mHistogramQualityPerDE->SetStats(0);
  getObjectsManager()->startPublishing(mHistogramQualityPerDE.get(), core::PublicationPolicy::ThroughStop);
  getObjectsManager()->setDefaultDrawOptions(mHistogramQualityPerDE.get(), "col");
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

void DigitsPostProcessing::updateRateHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(rateSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2FRatio* hr = obj->second.get<TH2FRatio>();
    if (hr) {
      mRatesPlotter->update(hr);
      if (mEnableLastCycleHistos) {
        // extract the average occupancies on the last cycle
        mElecMapOnCycle->update(hr);
        mRatesPlotterOnCycle->update(mElecMapOnCycle.get());
      }

      if (mEnableTrending) {
        auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
        if (mEnableLastCycleHistos) {
          mRatesTrendsPlotter->update(time, mElecMapOnCycle.get());
        } else {
          mRatesTrendsPlotter->update(time, hr);
        }
      }
    }
  }

  obj = mCcdbObjects.find(rateSignalSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2FRatio* hr = obj->second.get<TH2FRatio>();
    if (hr) {
      mRatesPlotterSignal->update(hr);
      if (mEnableLastCycleHistos) {
        // extract the average occupancies on the last cycle
        mElecMapSignalOnCycle->update(hr);
        mRatesPlotterSignalOnCycle->update(mElecMapSignalOnCycle.get());
      }

      if (mEnableTrending) {
        auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
        if (mEnableLastCycleHistos) {
          mRatesTrendsPlotterSignal->update(time, mElecMapSignalOnCycle.get());
        } else {
          mRatesTrendsPlotterSignal->update(time, hr);
        }
      }
    }
  }
}

//_________________________________________________________________________________________

void DigitsPostProcessing::updateOrbitHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(orbitsSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2F* hr = obj->second.get<TH2F>();
    if (hr) {
      mOrbitsPlotter->update(hr);
      if (mEnableLastCycleHistos) {
        // extract the average occupancies on the last cycle
        mDigitsOrbitsOnCycle->update(hr);
        mOrbitsPlotterOnCycle->update(mDigitsOrbitsOnCycle.get());
      }
    }
  }

  obj = mCcdbObjects.find(orbitsSignalSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2F* hr = obj->second.get<TH2F>();
    if (hr) {
      mOrbitsPlotterSignal->update(hr);
      if (mEnableLastCycleHistos) {
        // extract the average occupancies on the last cycle
        mDigitsSignalOrbitsOnCycle->update(hr);
        mOrbitsPlotterSignalOnCycle->update(mDigitsSignalOrbitsOnCycle.get());
      }
    }
  }
}

//_________________________________________________________________________________________

TH1* DigitsPostProcessing::getHistogram(std::string plotName)
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

void DigitsPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateRateHistos(t, &qcdb);
  updateOrbitHistos(t, &qcdb);

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

void DigitsPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
