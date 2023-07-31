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
/// \file   IDCsVsSACs.h
/// \author Bhawani Singh
///

#ifndef QUALITYCONTROL_IDCSVSSACS_H
#define QUALITYCONTROL_IDCSVSSACS_H

// O2 includes
#include "TPCCalibration/IDCContainer.h"
#include "TPCCalibration/IDCCCDBHelper.h"
#include "TPCCalibration/SACCCDBHelper.h"
#include "TPCCalibration/IDCGroupHelperSector.h"
#include "CCDB/CcdbApi.h"
#include "TPCQC/IDCsVsSACs.h"

// QC includes
#include "QualityControl/PostProcessingInterface.h"

// ROOT includes
#include "TCanvas.h"

#include <boost/property_tree/ptree_fwd.hpp>
#include <map>

namespace o2::quality_control_modules::tpc
{

/// \brief Quality Control task for the IDC data of the TPC comparision
/// \author Bhawani Singh
class IDCsVsSACs : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  IDCsVsSACs() = default;
  /// \brief Destructor
  ~IDCsVsSACs() = default;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param name     Name of the task
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
  o2::tpc::IDCCCDBHelper<unsigned char> mCCDBHelper;
  /// to have comparision plots of SACs and IDCs
  o2::tpc::SACCCDBHelper<unsigned char> mSACs;
  o2::tpc::qc::IDCsVsSACs mIDCsVsSACs;
  o2::ccdb::CcdbApi mCdbApi;
  std::string mHost;
  std::unique_ptr<TCanvas> mCompareIDC0andSAC0;

  std::unordered_map<std::string, long> mTimestamps;             ///< timestamps to look for specific data in the CCDB
  std::vector<std::map<std::string, std::string>> mLookupMaps{}; ///< meta data to look for data in the CCDB
  std::vector<std::map<std::string, std::string>> mStoreMaps{};  ///< meta data to be stored with the output in the QCDB
  std::unordered_map<std::string, std::vector<float>> mRanges;   ///< histogram ranges configurable via config file
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_IDCSVSSACS_H
