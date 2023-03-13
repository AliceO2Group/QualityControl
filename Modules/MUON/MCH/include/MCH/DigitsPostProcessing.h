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
/// \file    DigitsPostProcessing.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH digits
/// \since   21/06/2022
///

#ifndef QC_MODULE_MCH_PP_DIGITS_H
#define QC_MODULE_MCH_PP_DIGITS_H

#include "QualityControl/PostProcessingInterface.h"
#include "MCH/TH2ElecMapReductor.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/HistoOnCycle.h"
#include "MCH/RatesPlotter.h"
#include "MCH/RatesTrendsPlotter.h"
#include "MCH/OrbitsPlotter.h"
#include <TCanvas.h>

namespace o2::quality_control::core
{
class Activity;
}

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

/// \brief  A post-processing task which processes and trends MCH digits and produces plots.
class DigitsPostProcessing : public PostProcessingInterface
{
 public:
  DigitsPostProcessing() = default;
  ~DigitsPostProcessing() override = default;

  void configure(std::string name, const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  void createRatesHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void createOrbitHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateRateHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateOrbitHistos(Trigger t, repository::DatabaseInterface* qcdb);

  static std::string rateSourceName() { return "rate"; }
  static std::string rateSignalSourceName() { return "rate_signal"; }
  static std::string orbitsSourceName() { return "orbits"; }
  static std::string orbitsSignalSourceName() { return "orbits_signal"; }

  int64_t mRefTimeStamp;
  bool mFullHistos{ false };
  float mChannelRateMin;
  float mChannelRateMax;

  // CCDB object accessors
  std::map<std::string, CcdbObjectHelper> mCcdbObjects;
  std::map<std::string, CcdbObjectHelper> mCcdbObjectsRef;

  // Hit rate histograms ===============================================

  // On-cycle plots generators
  std::unique_ptr<HistoOnCycle<MergeableTH2Ratio>> mElecMapOnCycle;
  std::unique_ptr<HistoOnCycle<MergeableTH2Ratio>> mElecMapSignalOnCycle;
  // Plotters
  std::unique_ptr<RatesPlotter> mRatesPlotter;
  std::unique_ptr<RatesPlotter> mRatesPlotterOnCycle;
  std::unique_ptr<RatesPlotter> mRatesPlotterSignal;
  std::unique_ptr<RatesPlotter> mRatesPlotterSignalOnCycle;
  // Trends
  std::unique_ptr<RatesTrendsPlotter> mRatesTrendsPlotter;
  std::unique_ptr<RatesTrendsPlotter> mRatesTrendsPlotterSignal;

  // Time histograms ==============================================

  // On-cycle plots generators
  std::unique_ptr<HistoOnCycle<TH2F>> mDigitsOrbitsOnCycle;
  std::unique_ptr<HistoOnCycle<TH2F>> mDigitsSignalOrbitsOnCycle;
  // Plotters
  std::unique_ptr<OrbitsPlotter> mOrbitsPlotter;
  std::unique_ptr<OrbitsPlotter> mOrbitsPlotterOnCycle;
  std::unique_ptr<OrbitsPlotter> mOrbitsPlotterSignal;
  std::unique_ptr<OrbitsPlotter> mOrbitsPlotterSignalOnCycle;

  std::unique_ptr<TH2F> mHistogramQualityPerDE; ///< quality flags for each DE, to be filled by checker task
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_PP_DIGITS_H
