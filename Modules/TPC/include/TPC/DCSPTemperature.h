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
/// \file   DCSPTemperature.h
/// \author Thomas Klemenz
///

#ifndef QUALITYCONTROL_DCSPTEMPERATURE_H
#define QUALITYCONTROL_DCSPTEMPERATURE_H

// O2 includes
#include "CCDB/CcdbApi.h"
#include "TPCQC/DCSPTemperature.h"

// QC includes
#include "QualityControl/PostProcessingInterface.h"

#include <boost/property_tree/ptree_fwd.hpp>
#include <map>

namespace o2::quality_control_modules::tpc
{

/// \brief Quality Control task for the IDC data of the TPC
/// \author Thomas Klemenz
class DCSPTemperature : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  DCSPTemperature() = default;
  /// \brief Destructor
  ~DCSPTemperature() = default;

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

  /// \brief Splits strings
  /// Splits strings at delimiter and puts the tokens into a vector
  /// \param inString  String to be split
  /// \param delimiter  Delimiter where string should be split
  std::vector<std::string> splitString(const std::string inString, const char* delimiter);

  /// \brief Extracts the "Valid from" timestamp from metadata
  long getTimestamp(const std::string metaInfo);

  /// \brief Gives a vector of timestamps for data to be processed
  /// Gives a vector of time stamps of x files (x=nFiles) in path which are older than a given time stamp (limit)
  /// \param path  File path in the CCDB
  /// \param nFiles  Number of files that shall be processed
  /// \param limit  Most recent timestamp to be processed
  std::vector<long> getDataTimestamps(const std::string_view path, const unsigned int nFiles, const long limit);

 private:
  o2::tpc::qc::DCSPTemperature mDCSPTemp;
  o2::ccdb::CcdbApi mCdbApi;
  std::string mHost;
  int mNFiles;
  std::vector<std::unique_ptr<o2::tpc::dcs::Temperature>> mData;
  long mTimestamp;                               ///< timestamp to look for specific data in the CCDB
  std::map<std::string, std::string> mLookupMap; ///< meta data to look for data in the CCDB
  std::map<std::string, std::string> mStoreMap;  ///< meta data to be stored with the output in the QCDB
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_IDCS_H
