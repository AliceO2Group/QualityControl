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
/// \file    PreclustersPostProcessing.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH pre-clusters
/// \since   21/06/2022
///

#ifndef QC_MODULE_MCH_PP_PRECLUSTERS_H
#define QC_MODULE_MCH_PP_PRECLUSTERS_H

#include "QualityControl/PostProcessingInterface.h"

#include "MCH/Helpers.h"
#include "MCH/PostProcessingConfigMCH.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/HistoOnCycle.h"
#include "MCH/EfficiencyPlotter.h"
#include "MCH/EfficiencyTrendsPlotter.h"
#include "MCH/ClusterSizePlotter.h"
#include "MCH/ClusterSizeTrendsPlotter.h"
#include "MCH/ClusterChargePlotter.h"
#include "MCH/ClusterChargeTrendsPlotter.h"

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

/// \brief  A post-processing task which processes and trends MCH pre-clusters and produces plots.
class PreclustersPostProcessing : public PostProcessingInterface
{
 public:
  PreclustersPostProcessing() = default;
  ~PreclustersPostProcessing() override = default;

  void configure(std::string name, const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  /** create one histogram with relevant drawing options / stat box status.*/
  template <typename T>
  void publishHisto(T* h, bool statBox = false,
                    const char* drawOptions = "",
                    const char* displayHints = "");
  void createEfficiencyHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void createClusterChargeHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void createClusterSizeHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateEfficiencyHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateClusterChargeHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateClusterSizeHistos(Trigger t, repository::DatabaseInterface* qcdb);

  static std::string effSourceName() { return "eff"; }
  static std::string clusterChargeSourceName() { return "clcharge"; }
  static std::string clusterSizeSourceName() { return "clsize"; }

  // PreclustersConfig mConfig;
  int64_t mRefTimeStamp;
  bool mFullHistos{ false };

  // CCDB object accessors
  std::map<std::string, CcdbObjectHelper> mCcdbObjects;
  std::map<std::string, CcdbObjectHelper> mCcdbObjectsRef;

  // Hit rate histograms ===============================================

  // On-cycle plots generators
  std::unique_ptr<HistoOnCycle<MergeableTH2Ratio>> mElecMapOnCycle;
  std::unique_ptr<HistoOnCycle<TH2F>> mClusterChargeOnCycle;
  std::unique_ptr<HistoOnCycle<TH2F>> mClusterSizeOnCycle;
  // Plotters
  std::unique_ptr<EfficiencyPlotter> mEfficiencyPlotter;
  std::unique_ptr<EfficiencyPlotter> mEfficiencyPlotterOnCycle;

  std::unique_ptr<ClusterChargePlotter> mClusterChargePlotter;
  std::unique_ptr<ClusterChargePlotter> mClusterChargePlotterOnCycle;

  std::unique_ptr<ClusterSizePlotter> mClusterSizePlotter;
  std::unique_ptr<ClusterSizePlotter> mClusterSizePlotterOnCycle;

  std::unique_ptr<EfficiencyTrendsPlotter> mEfficiencyTrendsPlotter;
  std::unique_ptr<ClusterChargeTrendsPlotter> mClusterChargeTrendsPlotter;
  std::unique_ptr<ClusterSizeTrendsPlotter> mClusterSizeTrendsPlotter;

  std::unique_ptr<TH2F> mHistogramQualityPerDE; ///< quality flags for each DE, to be filled by checker task
};

template <typename T>
void PreclustersPostProcessing::publishHisto(T* h, bool statBox,
                                             const char* drawOptions,
                                             const char* displayHints)
{
  if (!statBox) {
    h->SetStats(0);
  }
  getObjectsManager()->startPublishing(h);
  if (drawOptions) {
    getObjectsManager()->setDefaultDrawOptions(h, drawOptions);
  }
  if (displayHints) {
    getObjectsManager()->setDisplayHint(h, displayHints);
  }
}

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_PP_PRECLUSTERS_H
