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
/// \file   SACs.h
/// \author Thomas Klemenz
///

#ifndef QUALITYCONTROL_SACS_H
#define QUALITYCONTROL_SACS_H

// O2 includes
#include "CCDB/CcdbApi.h"
#include "TPCQC/SACs.h"

// QC includes
#include "QualityControl/PostProcessingInterface.h"

// ROOT includes
#include "TCanvas.h"

#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::quality_control_modules::tpc
{

/// \brief Quality Control task for the IDC data of the TPC
/// \author Thomas Klemenz
class SACs : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  SACs() = default;
  /// \brief Destructor
  ~SACs() = default;

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
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  o2::tpc::qc::SACs mSACs;
  o2::ccdb::CcdbApi mCdbApi;
  std::string mHost;
  bool mDoLatest = false;
  std::unique_ptr<TCanvas> mSACZeroSides;
  std::unique_ptr<TCanvas> mSACOneSides;
  std::unique_ptr<TCanvas> mSACDeltaSides;
  std::unique_ptr<TCanvas> mFourierCoeffsA;
  std::unique_ptr<TCanvas> mFourierCoeffsC;

  std::unordered_map<std::string, long> mTimestamps;             ///< timestamps to look for specific data in the CCDB
  std::vector<std::map<std::string, std::string>> mLookupMaps{}; ///< meta data to look for data in the CCDB
  std::vector<std::map<std::string, std::string>> mStoreMaps{};  ///< meta data to be stored with the output in the QCDB
  std::unordered_map<std::string, std::vector<float>> mRanges;   ///< histogram ranges configurable via config file
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_IDCS_H