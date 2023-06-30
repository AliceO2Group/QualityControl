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
#include "MCH/PostProcessingConfigMCH.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include <TDatime.h>

using namespace o2::quality_control_modules::muonchambers;

void DigitsPostProcessing::configure(const boost::property_tree::ptree& config)
{
  PostProcessingConfigMCH mchConfig(getID(), config);

  mRefTimeStamp = mchConfig.getParameter<int64_t>("ReferenceTimeStamp", -1);
  auto refDate = mchConfig.getParameter<std::string>("ReferenceDate");
  if (!refDate.empty()) {
    TDatime date(refDate.c_str());
    mRefTimeStamp = date.Convert();
  }
  ILOG(Info, Devel) << "reference time stamp: " << mRefTimeStamp << "  (" << refDate << ")"
                    << AliceO2::InfoLogger::InfoLogger::endm;

  mFullHistos = mchConfig.getParameter<bool>("FullHistos", false);

  mChannelRateMin = mchConfig.getParameter<float>("ChannelRateMin", 0);
  mChannelRateMax = mchConfig.getParameter<float>("ChannelRateMax", 100);

  mCcdbObjects.emplace(rateSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(rateSignalSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(orbitsSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(orbitsSignalSourceName(), CcdbObjectHelper());

  if (mRefTimeStamp > 0) {
    mCcdbObjectsRef.emplace(rateSourceName(), CcdbObjectHelper());
    mCcdbObjectsRef.emplace(rateSignalSourceName(), CcdbObjectHelper());
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

void DigitsPostProcessing::createRatesHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(rateSourceName());
  if (obj != mCcdbObjects.end()) {
    mElecMapOnCycle = std::make_unique<HistoOnCycle<MergeableTH2Ratio>>();
  }

  obj = mCcdbObjects.find(rateSignalSourceName());
  if (obj != mCcdbObjects.end()) {
    mElecMapSignalOnCycle = std::make_unique<HistoOnCycle<MergeableTH2Ratio>>();
  }

  //----------------------------------
  // Reference mean rates histogram
  //----------------------------------

  TH2F* hElecHistoRef{ nullptr };
  obj = mCcdbObjectsRef.find(rateSourceName());
  if (obj != mCcdbObjectsRef.end() && obj->second.update(qcdb, mRefTimeStamp)) {
    ILOG(Info, Devel) << "Loaded reference plot \"" << obj->second.mObject->getName() << "\", time stamp " << mRefTimeStamp
                      << AliceO2::InfoLogger::InfoLogger::endm;
    hElecHistoRef = obj->second.get<TH2F>();
  } else {
    ILOG(Info, Devel) << "Could not load reference plot \"" << obj->second.mPath << "/" << obj->second.mName << "\", time stamp " << mRefTimeStamp
                      << AliceO2::InfoLogger::InfoLogger::endm;
  }

  TH2F* hElecSignalHistoRef{ nullptr };
  obj = mCcdbObjectsRef.find(rateSignalSourceName());
  if (obj != mCcdbObjectsRef.end() && obj->second.update(qcdb, mRefTimeStamp)) {
    ILOG(Info, Devel) << "Loaded reference plot \"" << obj->second.mObject->getName() << "\", time stamp " << mRefTimeStamp
                      << AliceO2::InfoLogger::InfoLogger::endm;
    hElecSignalHistoRef = obj->second.get<TH2F>();
  }

  //----------------------------------
  // Rate plotters
  //----------------------------------

  mRatesPlotter = std::make_unique<RatesPlotter>("Rates/", hElecHistoRef, mChannelRateMin, mChannelRateMax, mFullHistos);
  mRatesPlotter->publish(getObjectsManager());

  mRatesPlotterOnCycle = std::make_unique<RatesPlotter>("Rates/LastCycle/", hElecHistoRef, mChannelRateMin, mChannelRateMax, mFullHistos);
  mRatesPlotterOnCycle->publish(getObjectsManager());

  mRatesPlotterSignal = std::make_unique<RatesPlotter>("RatesSignal/", hElecSignalHistoRef, mChannelRateMin, mChannelRateMax, mFullHistos);
  mRatesPlotterSignal->publish(getObjectsManager());

  mRatesPlotterSignalOnCycle = std::make_unique<RatesPlotter>("RatesSignal/LastCycle/", hElecSignalHistoRef, mChannelRateMin, mChannelRateMax, mFullHistos);
  mRatesPlotterSignalOnCycle->publish(getObjectsManager());

  //----------------------------------
  // Rate trends
  //----------------------------------

  mRatesTrendsPlotter = std::make_unique<RatesTrendsPlotter>("Trends/Rates/", hElecHistoRef, mFullHistos);
  mRatesTrendsPlotter->publish(getObjectsManager());

  mRatesTrendsPlotterSignal = std::make_unique<RatesTrendsPlotter>("Trends/RatesSignal/", hElecSignalHistoRef, mFullHistos);
  mRatesTrendsPlotterSignal->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void DigitsPostProcessing::createOrbitHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(orbitsSourceName());
  if (obj != mCcdbObjects.end()) {
    mDigitsOrbitsOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
  }

  obj = mCcdbObjects.find(orbitsSignalSourceName());
  if (obj != mCcdbObjects.end()) {
    mDigitsSignalOrbitsOnCycle = std::make_unique<HistoOnCycle<TH2F>>();
  }

  //----------------------------------
  // Orbit plotters
  //----------------------------------

  mOrbitsPlotter = std::make_unique<OrbitsPlotter>("Orbits/");
  mOrbitsPlotter->publish(getObjectsManager());

  mOrbitsPlotterOnCycle = std::make_unique<OrbitsPlotter>("Orbits/LastCycle/");
  mOrbitsPlotterOnCycle->publish(getObjectsManager());

  mOrbitsPlotterSignal = std::make_unique<OrbitsPlotter>("OrbitsSignal/");
  mOrbitsPlotterSignal->publish(getObjectsManager());

  mOrbitsPlotterSignalOnCycle = std::make_unique<OrbitsPlotter>("OrbitsSignal/LastCycle/");
  mOrbitsPlotterSignalOnCycle->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void DigitsPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  createRatesHistos(t, &qcdb);
  createOrbitHistos(t, &qcdb);

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

void DigitsPostProcessing::updateRateHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(rateSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    MergeableTH2Ratio* hr = obj->second.get<MergeableTH2Ratio>();
    if (hr) {
      mRatesPlotter->update(hr);
      // extract the average occupancies on the last cycle
      mElecMapOnCycle->update(hr);
      mRatesPlotterOnCycle->update(mElecMapOnCycle.get());

      auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
      mRatesTrendsPlotter->update(time, mElecMapOnCycle.get());
    }
  }

  obj = mCcdbObjects.find(rateSignalSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    MergeableTH2Ratio* hr = obj->second.get<MergeableTH2Ratio>();
    if (hr) {
      mRatesPlotterSignal->update(hr);
      // extract the average occupancies on the last cycle
      mElecMapSignalOnCycle->update(hr);
      mRatesPlotterSignalOnCycle->update(mElecMapSignalOnCycle.get());

      auto time = obj->second.getTimeStamp() / 1000; // ROOT expects seconds since epoch
      mRatesTrendsPlotterSignal->update(time, mElecMapSignalOnCycle.get());
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
      // extract the average occupancies on the last cycle
      mDigitsOrbitsOnCycle->update(hr);
      mOrbitsPlotterOnCycle->update(mDigitsOrbitsOnCycle.get());
    }
  }

  obj = mCcdbObjects.find(orbitsSignalSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2F* hr = obj->second.get<TH2F>();
    if (hr) {
      mOrbitsPlotterSignal->update(hr);
      // extract the average occupancies on the last cycle
      mDigitsSignalOrbitsOnCycle->update(hr);
      mOrbitsPlotterSignalOnCycle->update(mDigitsSignalOrbitsOnCycle.get());
    }
  }
}

//_________________________________________________________________________________________

void DigitsPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateRateHistos(t, &qcdb);
  updateOrbitHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void DigitsPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
