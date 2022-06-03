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
/// \file   IDCs.h
/// \author Thomas Klemenz
///

#ifndef QUALITYCONTROL_IDCS_H
#define QUALITYCONTROL_IDCS_H

// O2 includes
#include "TPCCalibration/IDCContainer.h"
#include "TPCCalibration/IDCCCDBHelper.h"
#include "TPCCalibration/IDCGroupHelperSector.h"
#include "CCDB/CcdbApi.h"

// QC includes
#include "QualityControl/PostProcessingInterface.h"

// ROOT includes
#include "TCanvas.h"

#include <boost/property_tree/ptree_fwd.hpp>
#include <map>

namespace o2::quality_control_modules::tpc
{

/// \brief Quality Control task for the IDC data of the TPC
/// \author Thomas Klemenz
class IDCs : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  IDCs() = default;
  /// \brief Destructor
  ~IDCs() = default;

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
  o2::tpc::IDCCCDBHelper<float> mCCDBHelper;
  o2::ccdb::CcdbApi mCdbApi;
  std::string mHost;
  std::unique_ptr<TCanvas> mIDCZeroSides;
  std::unique_ptr<TCanvas> mIDCZeroRadialProf;
  std::unique_ptr<TCanvas> mIDCZeroStacksA;
  std::unique_ptr<TCanvas> mIDCZeroStacksC;
  std::unique_ptr<TCanvas> mIDCDeltaStacksA;
  std::unique_ptr<TCanvas> mIDCDeltaStacksC;
  std::unique_ptr<TCanvas> mIDCOneSides1D;
  std::unique_ptr<TCanvas> mFourierCoeffsA;
  std::unique_ptr<TCanvas> mFourierCoeffsC;
  std::unordered_map<std::string, long> mTimestamps;             ///< timestamps to look for specific data in the CCDB
  std::vector<std::map<std::string, std::string>> mLookupMaps{}; ///< meta data to look for data in the CCDB
  std::vector<std::map<std::string, std::string>> mStoreMaps{};  ///< meta data to be stored with the output in the QCDB
  std::unordered_map<std::string, std::vector<float>> mRanges;   ///< histogram ranges configurable via config file
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_IDCS_H
