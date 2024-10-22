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
/// \file   UserCodeInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/UserCodeInterface.h"
#include <DataFormatsCTP/CTPRateFetcher.h>
#include <thread>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseFactory.h"

using namespace o2::ccdb;
using namespace std;

namespace o2::quality_control::core
{

void UserCodeInterface::setCustomParameters(const CustomParameters& parameters)
{
  mCustomParameters = parameters;
  configure();
}

const std::string& UserCodeInterface::getName() const {
  return mName;
}

void UserCodeInterface::setName(const std::string& name) {
  mName = name;
}

void UserCodeInterface::enableCtpScalers(size_t runNumber, std::string ccdbUrl)
{
  // TODO bail if we are in async
  ILOG(Debug, Devel) << "Enabling CTP scalers" << ENDM;
  mCtpFetcher = make_shared<o2::ctp::CTPRateFetcher>();
  mScalersEnabled = true;
  auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
  ccdbManager.setURL(ccdbUrl);
  mCtpFetcher->setupRun(runNumber, &ccdbManager, /*1726300234140*/ getCurrentTimestamp(), false);

  mScalersLastUpdate = std::chrono::steady_clock::time_point::min();
  cout << "mScalersLastUpdate : " << std::chrono::duration_cast<std::chrono::nanoseconds>(mScalersLastUpdate.time_since_epoch()).count() << endl;
  getScalers(); // initial value
  ILOG(Debug, Devel) << "Enabled CTP scalers" << ENDM;
}

void UserCodeInterface::getScalers()
{
  if(!mScalersEnabled) {
    ILOG(Error, Ops) << "CTP scalers not enabled, impossible to get them." << ENDM;
    return; // TODO should we throw ? probably yes
  }
  ILOG(Debug, Devel) << "Updating scalers." << ENDM;

  if(! mDatabase) {
    ILOG(Error, Devel) << "Database not set ! Cannot update scalers." << ENDM;
    mScalersEnabled = false;

    return;
    // todo handle the case when database is not set
  }

  auto now = std::chrono::steady_clock::now();
  auto minutesSinceLast = std::chrono::duration_cast<std::chrono::minutes>(now - mScalersLastUpdate);
  cout << "now : " << std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() << endl;
  cout << "minutesSinceLast : " << minutesSinceLast << endl;

  // TODO get the interval from config
  if (minutesSinceLast.count() >= 0 /*first time it is neg*/ && minutesSinceLast.count() < 5) {
    ILOG(Debug, Devel) << "getScalers was called less than 5 minutes ago, use the cached value" << ENDM;
    return;
  }

  std::map<std::string, std::string> meta;
  void* rawResult = mDatabase->retrieveAny(typeid(o2::ctp::CTPRunScalers), "qc/CTP/Scalers", meta);
  o2::ctp::CTPRunScalers* ctpScalers = static_cast<o2::ctp::CTPRunScalers*>(rawResult);
  mCtpFetcher->updateScalers(*ctpScalers);
  mScalersLastUpdate = now;
  ILOG(Debug, Devel) << "Scalers updated." << ENDM;
}

double UserCodeInterface::getScalersValue(std::string sourceName, size_t runNumber)
{
  if(!mScalersEnabled) {
    ILOG(Error, Ops) << "CTP scalers not enabled, impossible to get the value." << ENDM;
    return 0;
  }
  getScalers(); // from QCDB
  auto& ccdbManager = o2::ccdb::BasicCCDBManager::instance();
  auto result = mCtpFetcher->fetchNoPuCorr(&ccdbManager, getCurrentTimestamp(), runNumber, sourceName);
  ILOG(Debug, Devel) << "Returning scaler value : " << result << ENDM;
  return result;
}

void UserCodeInterface::setDatabase(std::unordered_map<std::string, std::string> dbConfig)
{
  // TODO one could really argue that it would be easier to have a singleton for the QCDB... because here we will build and save a database object

  // TODO could we rather provide the general config to the User Code ? it would be useful for other stuff like the ccdb url

  cout << "dbConfig.count(\"implementation\") " << dbConfig.count("implementation")  << "    dbConfig.count(\"host\") : " << dbConfig.count("host") << endl;
  if(dbConfig.count("implementation") == 0 || dbConfig.count("host") == 0) {
    ILOG(Error, Devel) << "dbConfig is incomplete, we don't build the user code database instance " << ENDM;
    return;
    // todo
  }

  for (auto pair : dbConfig) {
    ILOG(Info,Devel) << pair.first << " : " << pair.second << ENDM;
  }

  // for every user code we instantiate.
  mDatabase = repository::DatabaseFactory::create(dbConfig.at("implementation"));
  mDatabase->connect(dbConfig);
  ILOG(Info, Devel) << "Database that is going to be used > Implementation : " << dbConfig.at("implementation") << " / Host : " << dbConfig.at("host") << ENDM;
}

} // namespace o2::quality_control::core