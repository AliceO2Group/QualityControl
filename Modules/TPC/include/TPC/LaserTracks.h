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
/// \file   LaserTracks.h
/// \author Thomas Klemenz
///

#ifndef QUALITYCONTROL_LASERTRACKS_H
#define QUALITYCONTROL_LASERTRACKS_H

// QC includes
#include "QualityControl/PostProcessingInterface.h"

#include <boost/property_tree/ptree_fwd.hpp>
#include <map>

class TCanvas;
class TPaveText;

namespace o2::quality_control_modules::tpc
{

/// \brief Quality Control task for the laser track calibration data of the TPC
/// \author Thomas Klemenz
class LaserTracks final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  LaserTracks() = default;
  /// \brief Destructor
  ~LaserTracks() = default;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param name     Name of the task
  /// \param config   ConfigurationInterface with prefix set to ""
  void configure(std::string name, const boost::property_tree::ptree& config) override;
  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;

 private:
  std::vector<std::unique_ptr<TCanvas>> mLaserTracksCanvasVec{}; ///< vector containing a vector of summary canvases for every CalDet object
  long mTimestamp = -1;                                          ///< timestamps to look for specific data in the CCDB
  std::map<std::string, std::string> mLookupMap{};               ///< meta data to look for data in the CCDB
  std::map<std::string, std::string> mStoreMap{};                ///< meta data to be stored with the output in the QCDB

  TPaveText* mNewZSCalibMsg = nullptr; ///< badge to indicate the necessity to upload new calibration data for ZS
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_LASERTRACKS_H
