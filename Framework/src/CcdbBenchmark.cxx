// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CcdbBenchmark.cxx
/// \author Barthelemy von Haller
///

#include "CcdbBenchmark.h"

#include <thread> // this_thread::sleep_for
#include <chrono>

#include <TH2F.h>
#include <QualityControl/CcdbDatabase.h>

#include <FairMQLogger.h>
#include <options/FairMQProgOptions.h> // device->fConfig

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseFactory.h"
#include "Common/Exceptions.h"

using namespace std;
using namespace std::chrono;
using namespace AliceO2::Common;
using namespace o2::quality_control::repository;
using namespace o2::monitoring;

namespace o2 {
namespace quality_control {
namespace core {

CcdbBenchmark::CcdbBenchmark()
  : mMaxIterations(0), mNumIterations(0), mSizeObjects(1), mNumberObjects(1), mTotalNumberObjects(0)
{
}

void CcdbBenchmark::InitTask()
{
  // parse arguments
  try {
    string db = fConfig->GetValue<string>("ccdb-url");
    mDatabase = dynamic_cast<CcdbDatabase *>(o2::quality_control::repository::DatabaseFactory::create("CCDB"));
    mDatabase->connect(db, "", "", "");
  } catch (boost::exception &exc) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n" << diagnostic << endl;
    if (diagnostic == "No diagnostic information available.") {
      throw;
    }
  }
  mMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
  mNumberObjects = fConfig->GetValue<uint64_t>("number-objects");
  mSizeObjects = fConfig->GetValue<uint64_t>("size-objects");
  mDeletionMode = static_cast<bool>(fConfig->GetValue<int>("delete"));
  mTaskName = fConfig->GetValue<string>("task-name");
  mObjectName = fConfig->GetValue<string>("object-name");
  auto numberTasks = fConfig->GetValue<uint64_t>("number-tasks");

  mMonitoring = MonitoringFactory::Get(fConfig->GetValue<string>("monitoring-url"));
  mMonitoring->enableProcessMonitoring(1); // collect every seconds metrics for this process
  mMonitoring->addGlobalTag("taskName", mTaskName);
  mMonitoring->addGlobalTag("numberObject", to_string(mNumberObjects));
  mMonitoring->addGlobalTag("sizeObject", to_string(mSizeObjects));
  if (mTaskName == "benchmarkTask_0") { // send these parameters to monitoring only once per benchmark run
    mMonitoring->sendGrouped("ccdb-benchmark-parameters", {{mNumberObjects, "number-objects"},
                                                            {mSizeObjects,   "size-objects"},
                                                            {numberTasks,    "number-tasks"}});
  }

  if (mDeletionMode) {
    QcInfoLogger::GetInstance() << "Deletion mode..." << infologger::endm;
    emptyDatabase();
  }

  // Prepare objects (and clean up existing ones)
  switch (mSizeObjects) {
    case 1:
      mMyHisto = new TH1F("h", "h", 100, 0, 99); // 1kB
      break;
    case 10:
      mMyHisto = new TH1F("h", "h", 2400, 0, 99); // 10kB
      break;
    case 100:
      mMyHisto = new TH2F("h", "h", 260, 0, 99, 100, 0, 99); // 100kB
      break;
    case 500:
      mMyHisto = new TH2F("h", "h", 1250, 0, 99, 100, 0, 99); // 500kB
      break;
    case 1000:
      mMyHisto = new TH2F("h", "h", 2500, 0, 99, 100, 0, 99); // 1MB
      break;
    case 2500:
      mMyHisto = new TH2F("h", "h", 6250, 0, 99, 100, 0, 99); // 2.5MB
      break;
    case 5000:
      mMyHisto = new TH2F("h", "h", 12500, 0, 99, 100, 0, 99); // 5MB
      break;
    default:
      BOOST_THROW_EXCEPTION(
        FatalException() << errinfo_details("size of histo must be 1, 10, 100, 500, 1000, 2500 or 5000 (was: " + to_string(mSizeObjects) + ")"));
  }
  for(uint64_t i = 0 ; i < mNumberObjects ; i++) {
    MonitorObject *mo = new MonitorObject(mObjectName+to_string(i), mMyHisto, mTaskName);
    mMyObjects.push_back(mo);
    mDatabase->truncateObject(mTaskName, mObjectName+to_string(i));
  }
}

bool CcdbBenchmark::ConditionalRun()
{
  if (mDeletionMode) { //the only way to not run is to return false from here.
    return false;
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  // Store the object
  for (unsigned int i = 0; i < mNumberObjects; i++) {
    mDatabase->store(mMyObjects[i]);
  }
  mTotalNumberObjects += mNumberObjects;
  mMonitoring->send({mTotalNumberObjects, "objectsSent"}, DerivedMetricMode::RATE);

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  long duration = duration_cast<milliseconds>(t2 - t1).count();
  mMonitoring->send({duration / mNumberObjects, "storeDurationForOneObject_ms"}, DerivedMetricMode::NONE);

  // determine how long we should wait till next iteration in order to have 1 sec between storage
  auto duration2 = duration_cast<microseconds>(t2 - t1);
  auto remaining = duration_cast<microseconds>(std::chrono::seconds(1) - duration2);
//  QcInfoLogger::GetInstance() << "Remaining duration : " << remaining.count() << " us" << infologger::endm;
  if (remaining.count() < 0) {
    QcInfoLogger::GetInstance() << "Remaining duration is negative, we don't sleep " << infologger::endm;
  } else {
    this_thread::sleep_for(chrono::microseconds(remaining));
  }

  if (mMaxIterations > 0 && ++mNumIterations >= mMaxIterations) {
    QcInfoLogger::GetInstance() << "Configured maximum number of iterations reached. Leaving RUNNING state."
                                << infologger::endm;
    return false;
  }

  return true;
}

void CcdbBenchmark::emptyDatabase()
{
  mDatabase->truncateObject(mTaskName, mObjectName);
}

}
}
}