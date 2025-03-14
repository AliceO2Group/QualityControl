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

const std::string& UserCodeInterface::getName() const
{
  return mName;
}

void UserCodeInterface::setName(const std::string& name)
{
  mName = name;
}

void UserCodeInterface::enableCtpScalers(size_t runNumber, std::string ccdbUrl)
{
  mCtpScalers.enableCtpScalers(runNumber, ccdbUrl);
}

double UserCodeInterface::getScalersValue(std::string sourceName, size_t runNumber)
{
  return mCtpScalers.getScalersValue( sourceName,  runNumber);
}

void UserCodeInterface::setDatabase(std::unordered_map<std::string, std::string> dbConfig)
{
  if (dbConfig.count("implementation") == 0 || dbConfig.count("host") == 0) {
    ILOG(Error, Devel) << "dbConfig is incomplete, we don't build the user code database instance" << ENDM;
    throw std::invalid_argument("Cannot set database in UserCodeInterface");
  }

  mDatabase = repository::DatabaseFactory::create(dbConfig.at("implementation"));
  mDatabase->connect(dbConfig);
  ILOG(Debug, Devel) << "Database that is going to be used > Implementation : " << dbConfig.at("implementation") << " / Host : " << dbConfig.at("host") << ENDM;

  mCtpScalers.setDatabase(mDatabase);
}

} // namespace o2::quality_control::core