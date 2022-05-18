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

#include <memory>
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
/// \brief  A post-processing task tuned for the needs of the trending of the TPC.
///
/// A post-processing task which trends TPC related objects inside QC database (QCDB).
/// It extracts some values of one or multiple objects using the Reductor classes,
/// then stores them inside a TTree.
/// This class is specific to the TPC: a subrange slicer is available in the json,
/// and input/output canvas can be dealt with alongside normal histograms.
///

class RatioGeneratorTPC : public PostProcessingInterface
{
 public:
  /// \brief Constructor.
  RatioGeneratorTPC() = default;
  /// \brief Destructor.
  ~RatioGeneratorTPC() final = default;

  /// \brief Post-processing methods inherited from 'PostProcessingInterface'.
  void configure(std::string name, const boost::property_tree::ptree& config) final;
  void initialize(Trigger, framework::ServiceRegistry&) final;
  void update(Trigger, framework::ServiceRegistry&) final;
  void finalize(Trigger, framework::ServiceRegistry&) final;

  struct dataSource {
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

  std::map<std::string, TH1*> mRatios;
  std::map<std::string, TCanvas*> mPlots;

  std::vector<dataSource> mConfig;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_RATIOGENERATORTPC_H
