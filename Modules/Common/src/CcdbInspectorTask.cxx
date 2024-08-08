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
/// \file    CcdbInspectorTask.cxx
/// \author  Andrea Ferrero
/// \brief   Post-processing task that checks the existence, time stamp and validity of CCDB/QCDB objects
///

#include "Common/CcdbInspectorTask.h"
#include "Common/CcdbValidatorInterface.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RootClassFactory.h"

#include <boost/property_tree/ptree.hpp>

#include <thread>
#include <chrono>

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::common
{

static std::string toString(CcdbInspectorTask::ObjectStatus status)
{
  switch (status) {
    case CcdbInspectorTask::ObjectStatus::objectStatusNotChecked:
      return "NotChecked";
      break;
    case CcdbInspectorTask::ObjectStatus::objectStatusMissing:
      return "Missing";
      break;
    case CcdbInspectorTask::ObjectStatus::objectStatusOld:
      return "Old";
      break;
    case CcdbInspectorTask::ObjectStatus::objectStatusInvalid:
      return "Invalid";
      break;
    case CcdbInspectorTask::ObjectStatus::objectStatusValid:
      return "Valid";
      break;
  }

  return "";
}

void CcdbInspectorTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = std::make_unique<CcdbInspectorTaskConfig>(getID(), config);

  for (const auto& dataSource : mConfig->dataSources) {
    if (dataSource.validatorName.empty() || dataSource.moduleName.empty()) {
      continue;
    }
    mValidators[dataSource.name].reset(root_class_factory::create<CcdbValidatorInterface>(dataSource.moduleName, dataSource.validatorName));
    ILOG(Info, Devel) << "Loaded CCDB validator module '" << dataSource.validatorName << " from '" << dataSource.moduleName << "'" << ENDM;
  }

  mDatabaseType = mCustomParameters.atOptional("databaseType").value_or(mDatabaseType);
  mDatabaseUrl = mCustomParameters.atOptional("databaseUrl").value_or(mDatabaseUrl);

  mTimeStampTolerance = std::stoi(mCustomParameters.atOptional("timeStampTolerance").value_or("60")) * 1000; // from seconds to milliseconds
  mRetryTimeout = std::stoi(mCustomParameters.atOptional("retryTimeout").value_or("60"));
  mRetryDelay = std::stoi(mCustomParameters.atOptional("retryDelay").value_or("10"));

  mDatabase = std::make_unique<CcdbDatabase>();
  mDatabase->connect(mDatabaseUrl, "", "", "");
  ILOG(Info, Devel) << "Initialized db connection to '" << mDatabaseUrl << "'" << ENDM;

  mHistObjectsStatus = std::make_unique<TH2F>("ObjectsStatus", "Objects Status", mConfig->dataSources.size(), 0, mConfig->dataSources.size(), 4, -4, 0);
  mHistObjectsStatus->SetMinimum(0);
  mHistObjectsStatus->SetMaximum(2);
  mHistObjectsStatus->SetOption("col");

  // set histogram bin labels
  for (auto& dataSource : mConfig->dataSources) {
    mHistObjectsStatus->GetXaxis()->SetBinLabel(dataSource.binNumber, dataSource.name.c_str());
  }
  mHistObjectsStatus->GetYaxis()->SetBinLabel(4, "not found");
  mHistObjectsStatus->GetYaxis()->SetBinLabel(3, "too old");
  mHistObjectsStatus->GetYaxis()->SetBinLabel(2, "invalid");
  mHistObjectsStatus->GetYaxis()->SetBinLabel(1, "ok");
}

//_________________________________________________________________________________________

void CcdbInspectorTask::initialize(Trigger trigger, framework::ServiceRegistryRef)
{
  for (auto& dataSource : mConfig->dataSources) {
    bool isPeriodic = dataSource.updatePolicy == CcdbInspectorTaskConfig::ObjectUpdatePolicy::updatePolicyPeriodic;
    // for each periodic data source, initialize the last know timestamp to the beginning of the processing,
    // such that the existance of the CCDB objects is not checked too early
    // for objects that are only created once, the last known time stamp is initialized to zero
    // such that the object is unconditionally searched
    if (isPeriodic) {
      dataSource.lastCreationTimestamp = trigger.timestamp;
    } else {
      dataSource.lastCreationTimestamp = 0;
    }
    dataSource.validObjectsCount = 0;
  }

  mHistObjectsStatus->Reset();

  getObjectsManager()->startPublishing(mHistObjectsStatus.get());
  getObjectsManager()->setDefaultDrawOptions(mHistObjectsStatus.get(), "col");
}

//_________________________________________________________________________________________

std::tuple<uint64_t, uint64_t, uint64_t, int> CcdbInspectorTask::getObjectInfo(const std::string& path, const std::map<std::string, std::string>& metadata)
{
  // find the time-stamp of the most recent object matching the current activity
  // if ignoreActivity is true the activity matching criteria are not applied

  auto listing = mDatabase->getListingAsPtree(path, metadata, true);
  if (listing.count("objects") == 0) {
    ILOG(Warning, Support) << "Could not get a valid listing from db '" << mDatabaseUrl << "' for latestObjectMetadata '" << path << "'" << ENDM;
    return std::make_tuple<uint64_t, uint64_t, uint64_t, int>(0, 0, 0, 0);
  }
  const auto& objects = listing.get_child("objects");
  if (objects.empty()) {
    ILOG(Warning, Devel) << "Could not find the file '" << path << "' in the db '"
                         << mDatabaseUrl << "' for given Activity settings. Zeroes and empty strings are treated as wildcards." << ENDM;
    return std::make_tuple<uint64_t, uint64_t, uint64_t, int>(0, 0, 0, 0);
  } else if (objects.size() > 1) {
    ILOG(Warning, Support) << "Expected just one metadata entry for object '" << path << "'. Trying to continue by using the first." << ENDM;
  }

  // retrieve timestamps
  const auto& latestObjectMetadata = objects.front().second;
  uint64_t creationTime = latestObjectMetadata.get<uint64_t>(metadata_keys::created);
  uint64_t validFrom = latestObjectMetadata.get<uint64_t>(metadata_keys::validFrom);
  uint64_t validUntil = latestObjectMetadata.get<uint64_t>(metadata_keys::validUntil);

  // retrieve 'runNumber' metadata element, or zero if not set
  uint64_t runNumber = latestObjectMetadata.get<int>("runNumber", 0);

  return std::make_tuple(validFrom, validUntil, creationTime, runNumber);
}

void* CcdbInspectorTask::getObject(const std::string& path, std::type_info const& tinfo, uint64_t timestamp, const std::map<std::string, std::string>& metadata)
{
  map<string, string> headers;
  void* obj = mDatabase->retrieveAny(tinfo, path, metadata, timestamp, &headers);
  ILOG(Debug, Devel) << "Retrieved '" << path << "' from db '" << mDatabaseUrl << "' for timestamp " << timestamp << ENDM;

  return obj;
}

CcdbInspectorTask::ObjectStatus CcdbInspectorTask::inspectObject(CcdbInspectorTaskConfig::DataSource& dataSource, Trigger trigger, bool atFinalize)
{
  auto path = dataSource.path;
  auto validatorName = dataSource.validatorName;
  auto moduleName = dataSource.moduleName;
  auto updatePolicy = dataSource.updatePolicy;
  auto cycleDuration = dataSource.cycleDuration * 1000; // in milliseconds
  auto lastCreationTimestamp = dataSource.lastCreationTimestamp;
  bool isPeriodic = updatePolicy == CcdbInspectorTaskConfig::ObjectUpdatePolicy::updatePolicyPeriodic;

  // skip check of periodic objects in the finalize() method, only check those that have EoR update policy
  if (atFinalize && updatePolicy != CcdbInspectorTaskConfig::ObjectUpdatePolicy::updatePolicyAtEOR) {
    ILOG(Debug, Devel) << "Not checking file '" << path << "' at finalize because policy is not 'At EoR'" << ENDM;
    return ObjectStatus::objectStatusNotChecked;
  }

  // skip objects that are expected only at the start or end of run, if they have been already found once
  if (dataSource.validObjectsCount > 0 && !isPeriodic) {
    ILOG(Debug, Devel) << "Skipping non-periodic file '" << path << "' because it was already found" << ENDM;
    return ObjectStatus::objectStatusNotChecked;
  }

  // skip objects that are expected only at the end of run if we are not in the finalize() method
  if (!atFinalize && updatePolicy == CcdbInspectorTaskConfig::ObjectUpdatePolicy::updatePolicyAtEOR) {
    ILOG(Debug, Devel) << "Not checking file '" << path << "' with policy 'At EoR'" << ENDM;
    return ObjectStatus::objectStatusNotChecked;
  }

  // don't check the object if we don't expect it to have been updated yet
  if (trigger.timestamp < (lastCreationTimestamp + cycleDuration)) {
    ILOG(Debug, Devel) << "Not checking file '" << path << "' with policy 'periodic' because elapsed "
                       << (trigger.timestamp - lastCreationTimestamp) / 1000 << " seconds is less than cycle duraction (" << dataSource.cycleDuration
                       << " seconds)" << ENDM;
    return ObjectStatus::objectStatusNotChecked;
  }

  // verbose messages, only for debugging
  if (isPeriodic) {
    ILOG(Debug, Devel) << "Checking file '" << path << "' with policy 'periodic' and cycle " << dataSource.cycleDuration
                       << " seconds, elapsed " << (trigger.timestamp - lastCreationTimestamp) / 1000 << " seconds" << ENDM;
  } else if (updatePolicy == CcdbInspectorTaskConfig::ObjectUpdatePolicy::updatePolicyAtSOR) {
    ILOG(Debug, Devel) << "Checking file '" << path << "' with policy 'At SoR',"
                       << " elapsed " << (trigger.timestamp - lastCreationTimestamp) / 1000 << " seconds" << ENDM;
  } else if (updatePolicy == CcdbInspectorTaskConfig::ObjectUpdatePolicy::updatePolicyAtEOR) {
    ILOG(Debug, Devel) << "Checking file '" << path << "' with policy 'At EoR',"
                       << " elapsed " << (trigger.timestamp - lastCreationTimestamp) / 1000 << " seconds" << ENDM;
  }

  // get timestamps and run numberof the last available object
  auto fullObjectPath = (mDatabaseType == "qcdb" ? trigger.activity.mProvenance + "/" : "") + path;
  // metadata for CCDB queries, only specifying the run number
  std::map<std::string, std::string> metadataCcdb{ { "runNumber", std::to_string(trigger.activity.mId) } };
  auto metadata = mDatabaseType == "qcdb" ? activity_helpers::asDatabaseMetadata(trigger.activity, false) : metadataCcdb;
  auto timestamps = getObjectInfo(path, metadata);
  auto creationTime = std::get<2>(timestamps);
  auto runNumber = std::get<3>(timestamps);

  // the object is considered found if the associated run number (if valid) matches the current one,
  // and if the creation time is newer that the last known timestamp
  bool isFound = (runNumber <= 0 || trigger.activity.mId == runNumber) && (creationTime > lastCreationTimestamp);
  if (!isFound) {
    ILOG(Warning, Support) << "File '" << path << "' for run " << trigger.activity.mId << " from db '" << mDatabaseUrl << "' not found" << ENDM;

    // run numbers do not match
    if (runNumber > 0 && trigger.activity.mId != runNumber) {
      ILOG(Warning, Support) << "Mismatching run numbers for file '" << path << ": "
                             << runNumber << " != " << trigger.activity.mId << ENDM;
    }

    // object was created in the past
    if (creationTime < lastCreationTimestamp) {
      ILOG(Warning, Support) << "File '" << path << "' is older than " << lastCreationTimestamp << ENDM;
    }
  } else {
    ILOG(Info, Devel) << "File '" << path << "' found in the db '" << mDatabaseUrl << "' with creationTime=" << creationTime << " and runNumber=" << runNumber
                      << " for activity " << trigger.activity << ENDM;
  }

  // The object is considered old if the timestamp is more than one  cycle duration in the past.
  // We add some tolerance to compensate possible variations in the creation time stamp
  // This condition is ignored when checking non-periodic objects, since in this case we just
  // need to know that there was one object created after the start of run
  bool isOld = isPeriodic ? (creationTime < (lastCreationTimestamp + cycleDuration - mTimeStampTolerance)) && (isFound) : false;

  if (isOld) {
    ILOG(Warning, Support) << "File '" << path << "' for run " << trigger.activity.mId << " from db '" << mDatabaseUrl << "' is too old" << ENDM;
  }

  bool isValid = true;
  if (isFound && !isOld) {
    // creation time is valid, update the last known timestamp of this data source
    dataSource.lastCreationTimestamp = creationTime;
    dataSource.validObjectsCount += 1;
    ILOG(Debug, Devel) << "Found file '" << path << "' from db '" << mDatabaseUrl << "' for run " << trigger.activity.mId
                       << " creationTime=" << creationTime << ENDM;

    // run the validator on the object, skip if no validator is associated to this data source
    // only executed if the time stamp of the object is valid
    auto validatorIter = mValidators.find(dataSource.name);
    if (validatorIter != mValidators.end() && validatorIter->second) {
      auto timestamp = std::get<1>(timestamps) - 1;
      void* obj = getObject(fullObjectPath, validatorIter->second->getTinfo(), timestamp, metadata);
      ILOG(Debug, Devel) << "Retrieved object for file '" << path << "' from db '" << mDatabaseUrl << "' for timestamp " << timestamp
                         << " obj=" << obj << ENDM;
      isValid = validatorIter->second->validate(obj);
      ILOG(Debug, Devel) << "Validated object for file '" << path << "' from db '" << mDatabaseUrl << "' for timestamp " << timestamp
                         << " isValid=" << isValid << ENDM;
    }
  }

  ObjectStatus objectStatus = ObjectStatus::objectStatusNotChecked;

  if (!isFound) {
    objectStatus = ObjectStatus::objectStatusMissing;
  } else if (isOld) {
    objectStatus = ObjectStatus::objectStatusOld;
  } else if (!isValid) {
    objectStatus = ObjectStatus::objectStatusInvalid;
  } else {
    objectStatus = ObjectStatus::objectStatusValid;
  }
  ILOG(Info, Devel) << "File '" << path << "' from db '" << mDatabaseUrl << "': status is " << toString(objectStatus) << ENDM;

  // update contents of bin associated to current data source
  for (int biny = 1; biny <= mHistObjectsStatus->GetYaxis()->GetNbins(); biny++) {
    mHistObjectsStatus->SetBinContent(dataSource.binNumber, biny, 0);
  }
  mHistObjectsStatus->SetBinContent(dataSource.binNumber, static_cast<int>(objectStatus) + 1, 1);

  return objectStatus;
}

void CcdbInspectorTask::update(Trigger t, framework::ServiceRegistryRef)
{
  for (auto& dataSource : mConfig->dataSources) {
    inspectObject(dataSource, t, false);
  }
}

//_________________________________________________________________________________________

void CcdbInspectorTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Devel) << "Checking objects at finalize() " << ENDM;
  int elapsed = 0;
  while (elapsed < mRetryTimeout) {
    bool allFound = true;
    for (auto& dataSource : mConfig->dataSources) {
      auto status = inspectObject(dataSource, t, true);
      if (static_cast<int>(status) > 0) {
        allFound = false;
      }
    }

    if (allFound) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(mRetryDelay));
    elapsed += mRetryDelay;
  }
}

} // namespace o2::quality_control_modules::common
