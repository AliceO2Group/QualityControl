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
/// \file   Actor.cxx
/// \author Piotr Konopka
///

#include "QualityControl/Actor.h"

#include <boost/exception/diagnostic_information.hpp>
#include <Monitoring/Monitoring.h>
#include <Monitoring/MonitoringFactory.h>
#include <CCDB/BasicCCDBManager.h>
#include <Framework/RuntimeError.h>

#include "QualityControl/Bookkeeping.h"
#include "QualityControl/DatabaseFactory.h"

namespace o2::quality_control::core
{

namespace impl
{
std::shared_ptr<monitoring::Monitoring> initMonitoring(std::string_view url, std::string_view detector)
{
  auto monitoring = monitoring::MonitoringFactory::Get(std::string{url});
  monitoring->addGlobalTag(monitoring::tags::Key::Subsystem, monitoring::tags::Value::QC);
  // mMonitoring->addGlobalTag("TaskName", mTaskConfig.taskName);
  if (!detector.empty()) {
    monitoring->addGlobalTag("DetectorName", detector);
  }

  return std::move(monitoring);
}

void startMonitoring(monitoring::Monitoring& monitoring, int runNumber)
{
  monitoring.setRunNumber(runNumber);
}

void initBookkeeping(std::string_view url)
{
  Bookkeeping::getInstance().init(url.data());
}

void startBookkeeping(int runNumber, std::string_view actorName, std::string_view detectorName, const o2::bkp::DplProcessType& processType, std::string_view args)
{
  Bookkeeping::getInstance().registerProcess(runNumber, actorName.data(), detectorName.data(), processType, args.data());
}

Bookkeeping& getBookkeeping()
{
  return Bookkeeping::getInstance();
}

std::shared_ptr<quality_control::repository::DatabaseInterface> initRepository(const std::unordered_map<std::string, std::string>& config)
{
  auto db = quality_control::repository::DatabaseFactory::create(config.at("implementation"));
  assert(db != nullptr);
  db->connect(config);
  ILOG(Info, Devel) << "Database that is going to be used > Implementation : " << config.at("implementation") << " / Host : " << config.at("host") << ENDM;
  return std::move(db);
}

void initCCDB(const std::string& url)
{
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setURL(url);
  mgr.setFatalWhenNull(false);
}

ccdb::CCDBManagerInstance& getCCDB()
{
  return o2::ccdb::BasicCCDBManager::instance();
}

void handleExceptions(std::string_view when, const std::function<void()>& f)
{
  try {
    f();
  } catch (o2::framework::RuntimeErrorRef& ref) {
    ILOG(Error) << "Error occurred during " << when << ": " << o2::framework::error_from_ref(ref).what << ENDM;
    throw;
  } catch (...) {
    ILOG(Error) << "Error occurred during " << when << " :"
                << boost::current_exception_diagnostic_information(true) << ENDM;
    throw;
  }
}

}

}
