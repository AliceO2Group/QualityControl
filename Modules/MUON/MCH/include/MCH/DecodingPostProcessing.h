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
/// \file    DecodingPostProcessing.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Trending of the MCH tracking
/// \since   21/06/2022
///

#ifndef QC_MODULE_MCH_PP_DIAGNOSTICS_H
#define QC_MODULE_MCH_PP_DIAGNOSTICS_H

#include "QualityControl/PostProcessingInterface.h"

#include "MCH/Helpers.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/HistoOnCycle.h"
#include "MCH/DecodingErrorsPlotter.h"
#include "MCH/HeartBeatPacketsPlotter.h"
#include "MCH/FECSyncStatusPlotter.h"

#include <memory>

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

/// \brief  A post-processing task which trends MCH hits and stores them in a TTree and produces plots.
class DecodingPostProcessing : public PostProcessingInterface
{
 public:
  DecodingPostProcessing() = default;
  ~DecodingPostProcessing() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  /** create one histogram with relevant drawing options / stat box status.*/
  template <typename T>
  void publishHisto(T* h, bool statBox = false,
                    const char* drawOptions = "",
                    const char* displayHints = "");

  void createDecodingErrorsHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void createHeartBeatPacketsHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void createSyncStatusHistos(Trigger t, repository::DatabaseInterface* qcdb);

  void updateDecodingErrorsHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateHeartBeatPacketsHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateSyncStatusHistos(Trigger t, repository::DatabaseInterface* qcdb);

  static std::string errorsSourceName() { return "errors"; }
  static std::string hbPacketsSourceName() { return "hbpackets"; }
  static std::string syncStatusSourceName() { return "syncstatus"; }

  int64_t mRefTimeStamp;
  bool mFullHistos{ false };

  // CCDB object accessors
  std::map<std::string, CcdbObjectHelper> mCcdbObjects;

  // Decoding errors histograms ===============================================

  // On-cycle plots generators
  std::unique_ptr<HistoOnCycle<MergeableTH2Ratio>> mErrorsOnCycle;
  std::unique_ptr<HistoOnCycle<MergeableTH2Ratio>> mHBPacketsOnCycle;
  std::unique_ptr<HistoOnCycle<MergeableTH2Ratio>> mSyncStatusOnCycle;
  // Plotters and histograms
  std::unique_ptr<DecodingErrorsPlotter> mErrorsPlotter;
  std::unique_ptr<DecodingErrorsPlotter> mErrorsPlotterOnCycle;
  std::unique_ptr<HeartBeatPacketsPlotter> mHBPacketsPlotter;
  std::unique_ptr<HeartBeatPacketsPlotter> mHBPacketsPlotterOnCycle;
  std::unique_ptr<FECSyncStatusPlotter> mSyncStatusPlotter;
  std::unique_ptr<FECSyncStatusPlotter> mSyncStatusPlotterOnCycle;
};

template <typename T>
void DecodingPostProcessing::publishHisto(T* h, bool statBox,
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

#endif // QC_MODULE_MCH_PP_DIGITS_H
