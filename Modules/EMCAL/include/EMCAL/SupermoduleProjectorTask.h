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
/// \file   SupermoduleProjectorTask.h
/// \author Cristina Terrevoli, Markus Fasel
///

#ifndef QUALITYCONTROL_SUPERMODULEPROJECTORTASK_H
#define QUALITYCONTROL_SUPERMODULEPROJECTORTASK_H

// QC includes
#include "QualityControl/QualityObject.h"
#include "QualityControl/PostProcessingInterface.h"
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <cfloat>

class TCanvas;
class TPaveText;
class TH1;
class TH2;

namespace o2::quality_control_modules::emcal
{

/// \brief Quality Control task for the calibration data of the EMCAL
class SupermoduleProjectorTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  // ported from TrendingConfiguration
  struct DataSource {
    std::string type;
    std::string path;
    std::string name;
  };
  struct PlotAttributes {
    std::string titleX;
    std::string titleY;
    double minX = DBL_MIN;
    double maxX = DBL_MAX;
    bool logx = false;
    bool logy = false;
    std::string qualityPath;
  };

  /// \brief Constructor
  SupermoduleProjectorTask() = default;
  /// \brief Destructor
  ~SupermoduleProjectorTask() = default;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param config   ConfigurationInterface with prefix set to ""
  void configure(const boost::property_tree::ptree& config) override;
  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef services) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

  void reset();

 private:
  std::vector<DataSource> getDataSources(std::string name, const boost::property_tree::ptree& config);
  std::map<std::string, PlotAttributes> parseCustomizations(std::string name, const boost::property_tree::ptree& config);
  std::map<int, std::string> parseQuality(const quality_control::core::QualityObject& qo) const;

  /// \brief Make projections of the monitoring object per supermodule
  /// \param mo Monitoring object to project
  /// \param plot Canvas to plot the projections on
  /// \param customizations Plotting customizations for all pads (optional)
  void makeProjections(quality_control::core::MonitorObject& mo, TCanvas& plot, const PlotAttributes* customizations = nullptr, const quality_control::core::QualityObject* qo = nullptr);

  std::vector<DataSource> mDataSources;                    ///< Data sources to be projected
  std::map<std::string, TCanvas*> mCanvasHandler;          ///< Mapping between data source and output canvas
  std::map<std::string, PlotAttributes> mAttributeHandler; ///< Customizations for canvases (i.e. axis titles)
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_CALIBMONITORINGTASK_H
