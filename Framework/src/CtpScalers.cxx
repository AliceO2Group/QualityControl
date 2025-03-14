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
/// \file   CtpScalers.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CtpScalers.h"
#include <DataFormatsCTP/CTPRateFetcher.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseFactory.h"

using namespace o2::ccdb;
using namespace std;

namespace o2::quality_control::core
{
void CtpScalers::enableCtpScalers(size_t runNumber, std::string ccdbUrl)
{
  // bail if we are in async
  auto deploymentMode = framework::DefaultsHelpers::deploymentMode();
  if (deploymentMode == framework::DeploymentMode::Grid) {
    ILOG(Info, Ops) << "Async mode detected, CTP scalers cannot be enabled." << ENDM;
    return;
  }

  ILOG(Debug, Devel) << "Enabling CTP scalers" << ENDM;
  mCtpFetcher = make_shared<o2::ctp::CTPRateFetcher>();
  mScalersEnabled = true;
  auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
  ccdbManager.setURL(ccdbUrl);
  mCtpFetcher->setupRun(runNumber, &ccdbManager, /*1726300234140*/ getCurrentTimestamp(), false);

  mScalersLastUpdate = std::chrono::steady_clock::time_point::min();
  if (updateScalers(runNumber)) { // initial value
    ILOG(Debug, Devel) << "Enabled CTP scalers" << ENDM;
  } else {
    ILOG(Debug, Devel) << "CTP scalers not enabled, failure to get them." << ENDM;
  }
}

bool CtpScalers::updateScalers(size_t runNumber)
{
  if (!mScalersEnabled) {
    ILOG(Error, Ops) << "CTP scalers not enabled, impossible to update them." << ENDM;
    return false;
  }
  ILOG(Debug, Devel) << "Updating scalers." << ENDM;

  if (!mDatabase) {
    ILOG(Error, Devel) << "Database not set ! Cannot update scalers." << ENDM;
    mScalersEnabled = false;
    return false;
  }

  auto now = std::chrono::steady_clock::now();
  auto minutesSinceLast = std::chrono::duration_cast<std::chrono::minutes>(now - mScalersLastUpdate);

  // TODO get the interval from config
  if (minutesSinceLast.count() >= 0 /*first time it is neg*/ && minutesSinceLast.count() < 5) {
    ILOG(Debug, Devel) << "getScalers was called less than 5 minutes ago, use the cached value" << ENDM;
    return true;
  }

  std::map<std::string, std::string> meta;
  meta["runNumber"] = std::to_string(runNumber);
  std::map<std::string, std::string> headers;
  auto validity = mDatabase->getLatestObjectValidity("qc/CTP/Scalers", meta);
  void* rawResult = mDatabase->retrieveAny(typeid(o2::ctp::CTPRunScalers), "qc/CTP/Scalers", meta, validity.getMax() - 1, &headers);
  if (!rawResult) {
    ILOG(Error, Devel) << "Could not retrieve the CTP Scalers" << ENDM;
    return false;
  } else {
    ILOG(Debug, Devel) << "object retrieved" << ENDM;
  }

  o2::ctp::CTPRunScalers* ctpScalers = static_cast<o2::ctp::CTPRunScalers*>(rawResult);
  mCtpFetcher->updateScalers(*ctpScalers);
  mScalersLastUpdate = now;
  ILOG(Debug, Devel) << "Scalers updated." << ENDM;
  return true;
}

double CtpScalers::getScalersValue(std::string sourceName, size_t runNumber)
{
  if (!mScalersEnabled) {
    ILOG(Error, Ops) << "CTP scalers not enabled, impossible to get the value." << ENDM;
    return 0;
  }
  if (!updateScalers(runNumber)) { // from QCDB
    ILOG(Debug, Devel) << "Could not update the scalers, returning 0" << ENDM;
    return 0;
  }
  auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
  auto result = mCtpFetcher->fetchNoPuCorr(&ccdbManager, getCurrentTimestamp() * 1000, runNumber, sourceName);
  ILOG(Debug, Devel) << "Returning scaler value : " << result << ENDM;
  return result;
}

} // namespace o2::quality_control::core