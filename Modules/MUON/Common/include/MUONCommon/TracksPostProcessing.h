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
/// \file    TracksPostProcessing.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MUON tracks
///

#ifndef QC_MODULE_MUON_COMMON_PP_TRACKS_H
#define QC_MODULE_MUON_COMMON_PP_TRACKS_H

#include "MUONCommon/HistPlotter.h"
#include "MUONCommon/TracksPostProcessingConfig.h"
#include "QualityControl/PostProcessingInterface.h"
#include "ReconstructionDataFormats/GlobalTrackID.h"
#include <TH1D.h>
#include <array>
#include <map>

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

namespace o2::quality_control_modules::muon
{

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;
class MatchingEfficiencyPlotterInterface;

/// \brief  A post-processing task which processes and trends MCH digits and produces plots.
class TracksPostProcessing : public PostProcessingInterface
{
 public:
  TracksPostProcessing() = default;
  ~TracksPostProcessing() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  void createTrackHistos();
  void removeTrackHistos();
  void updateTrackHistos(Trigger t, repository::DatabaseInterface* qcdb);

  std::unique_ptr<TracksPostProcessingConfig> mConfig;

  std::vector<std::shared_ptr<MatchingEfficiencyPlotterInterface>> mMatchingEfficiencyPlotters;
};

} // namespace o2::quality_control_modules::muon

#endif // QC_MODULE_MUON_COMMON_PP_TRACKS_H
