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
/// \file     RatioGeneratorTPC.h
/// \author   Marcel Lesch
///

#ifndef QUALITYCONTROL_RATIOGENERATORTPC_H
#define QUALITYCONTROL_RATIOGENERATORTPC_H

#include "QualityControl/PostProcessingInterface.h"
#include <unordered_map>
#include <vector>
#include <TH1.h>
#include <TCanvas.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
} // namespace o2::quality_control::repository

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::tpc
{
/// \brief  A post-processing task which generates ratios of two merged histogram for post-processing
///
/// A post-processing task which generates ratios of two histogram for post-processing.
/// It takes two TH1 objects as input, calculates the ratio and stores the ratio TH1 on ccdb/qcg.
///

class RatioGeneratorTPC : public PostProcessingInterface
{
 public:
  /// \brief Constructor.
  RatioGeneratorTPC() = default;
  /// \brief Destructor.
  ~RatioGeneratorTPC() final = default;

  /// \brief Post-processing methods inherited from 'PostProcessingInterface'.
  void configure(const boost::property_tree::ptree& config) final;
  void initialize(Trigger, framework::ServiceRegistryRef) final{};
  void update(Trigger, framework::ServiceRegistryRef) final;
  void finalize(Trigger, framework::ServiceRegistryRef) final;

  struct DataSource {
    std::string path;
    std::string nameInputObjects[2];
    std::string nameOutputObject;
    std::string plotTitle;
    std::string axisTitle;
  };

 private:
  /// \brief Method to calculate the ratio between two TH1.
  void generateRatios(const Trigger& t, o2::quality_control::repository::DatabaseInterface&);
  /// \brief Method to create and publish plot.
  void generatePlots();

  std::unordered_map<std::string, TH1*> mRatios;
  std::vector<DataSource> mConfig;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_RATIOGENERATORTPC_H
