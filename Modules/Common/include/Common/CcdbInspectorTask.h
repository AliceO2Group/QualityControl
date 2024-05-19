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
#include "QualityControl/CcdbDatabase.h"

#include <TH2F.h>

#include <tuple>

namespace o2::quality_control_modules::common
{

class CcdbValidatorInterface;

/// \brief  Post-processing task that checks the existence, time stamp and validity of CCDB/QCDB objects
/// \author Andrea Ferrero
class CcdbInspectorTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// possible results of the object inspection
  enum class ObjectStatus {
    objectStatusNotChecked = -1, ///< the check was skipped
    objectStatusValid = 0,       ///< object exists and is valid
    objectStatusInvalid = 1,     ///< object exists but is invalid
    objectStatusOld = 2,         ///< the last version of the object is older than expected
    objectStatusMissing = 3      ///< object cannot be found
  };

  CcdbInspectorTask() = default;
  ~CcdbInspectorTask() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  /// returns the creation time stamp and the run number associated to the object described by "path"
  /// \param path the path of the object in the DB
  /// \param metadata optional metadata descriptors that have to be matched when searching the object
  /// \return a tuple providing the object's time-stamps and associated run number (<0>=validFrom, <1>=validUntil, <2>=creationTime, <3>=runNumber)
  std::tuple<uint64_t, uint64_t, uint64_t, int> getObjectInfo(const std::string& path, const std::map<std::string, std::string>& metadata);
  /// returns a copy of the object described by "path" and valid for a given time-stamp
  /// \param path the path of the object in the DB
  /// \param tinfo information about the type of the object to be retrieved
  /// \param timestamp to be matched with the object's validity
  /// \param metadata optional metadata descriptors that have to be matched when searching the object
  /// \return the address of the retrieved object, or nulptr if not found
  void* getObject(const std::string& path, std::type_info const& tinfo, uint64_t timestamp, const std::map<std::string, std::string>& metadata);
  /// searches the object described by the data source and compatible with the time-stamp and activity associated to the trigger
  /// \param dataSource the data source describing the object's path and other relevant parameters
  /// \param trigger the post-processing trigger with the associated time-stamp and activity
  /// \param atFinalize flag indicating wether the function has been called from the finalize() method
  ObjectStatus inspectObject(CcdbInspectorTaskConfig::DataSource& dataSource, Trigger trigger, bool atFinalize);

  /// tolerance on the creation time stamp of the objects
  int mTimeStampTolerance{ 60000 }; ///< in milliseconds
  /// timeout for object query retries at finlaize
  int mRetryTimeout{ 60 };
  /// delay between object query retries at finlaize
  int mRetryDelay{ 10 };
  /// type and address of the source database
  std::string mDatabaseType{ "ccdb" };
  std::string mDatabaseUrl{ "https://alice-ccdb.cern.ch" };
  std::unique_ptr<o2::quality_control::repository::CcdbDatabase> mDatabase;
  std::unique_ptr<CcdbInspectorTaskConfig> mConfig;
  /// external validator modules to inspect the contents of the DB objects
  std::unordered_map<std::string, std::shared_ptr<CcdbValidatorInterface>> mValidators;
  /// output plot that summarizes the status of the monitored DB objects
  std::unique_ptr<TH2F> mHistObjectsStatus;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_CCDB_INSPECTOR_TASK_H
