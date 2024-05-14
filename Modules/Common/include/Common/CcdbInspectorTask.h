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
/// \file    CcdbInspectorTask.h
/// \author  Andrea Ferrero
/// \brief   Post-processing task that checks the existence, time stamp and validity of CCDB/QCDB objects
///

#ifndef QUALITYCONTROL_CCDB_INSPECTOR_TASK_H
#define QUALITYCONTROL_CCDB_INSPECTOR_TASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "Common/CcdbInspectorTaskConfig.h"
#include "QualityControl/CcdbValidatorInterface.h"
#include "QualityControl/CcdbDatabase.h"

#include <TH2F.h>

#include <tuple>

namespace o2::quality_control_modules::common
{

/// \brief  Post-processing task that checks the existence, time stamp and validity of CCDB/QCDB objects
/// \author Andrea Ferrero
class CcdbInspectorTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  CcdbInspectorTask() = default;
  ~CcdbInspectorTask() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  std::tuple<uint64_t, uint64_t, uint64_t, int> getObjectTimestamps(const std::string& path, const std::map<std::string, std::string>& metadata);
  void* getObject(const std::string& path, std::type_info const& tinfo, uint64_t timestamp, const std::map<std::string, std::string>& metadata);
  int inspectSource(CcdbInspectorTaskConfig::DataSource& dataSource, Trigger t, bool atFinalize);

  ///< tolerance on the creation time stamp of the objects
  int mTimeStampTolerance{ 60000 }; // in milliseconds
  ///< timeout for object query retries at finlaize
  int mRetryTimeout{ 60 };
  ///< delay between object query retries at finlaize
  int mRetryDelay{ 10 };
  ///< print additional debuggin messages
  bool mVerbose{ false };
  ///< type and address of the source database
  std::string mDatabaseType{ "ccdb" };
  std::string mDatabaseUrl{ "https://alice-ccdb.cern.ch" };
  std::unique_ptr<o2::quality_control::repository::CcdbDatabase> mDatabase;
  std::unique_ptr<CcdbInspectorTaskConfig> mConfig;
  ///< external validator modules to inspect the contents of the DB objects
  std::unordered_map<std::string, std::unique_ptr<CcdbValidatorInterface>> mValidators;
  ///< output plot that summarizes the status of the monitored DB objects
  std::unique_ptr<TH2F> mHistObjectsStatus;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_CCDB_INSPECTOR_TASK_H
