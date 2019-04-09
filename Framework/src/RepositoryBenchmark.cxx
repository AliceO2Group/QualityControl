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
/// \file   RepositoryBenchmark.cxx
/// \author Barthelemy von Haller
///

#include "RepositoryBenchmark.h"

#include <chrono>
#include <thread> // this_thread::sleep_for

#include <QualityControl/CcdbDatabase.h>
#include <TH2F.h>

#include <FairMQLogger.h>
#include <options/FairMQProgOptions.h> // device->fConfig

#include "Common/Exceptions.h"
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;
using namespace std::chrono;
using namespace AliceO2::Common;
using namespace o2::quality_control::repository;
using namespace o2::monitoring;

namespace o2::quality_control::core
{

RepositoryBenchmark::RepositoryBenchmark()
  : mMaxIterations(0),
    mNumIterations(0),
    mNumberObjects(1),
    mSizeObjects(1),
    mThreadedMonitoring(true),
    mTotalNumberObjects(0)
{
}

TH1* RepositoryBenchmark::createHisto(uint64_t sizeObjects, string name)
{
  TH1* myHisto;

  // Prepare objects (and clean up existing ones)
  switch (sizeObjects) {
    case 1:
      myHisto = new TH1F(name.c_str(), "h", 100, 0, 99); // 1kB
      break;
    case 10:
      myHisto = new TH1F(name.c_str(), "h", 2400, 0, 99); // 10kB
      break;
    case 100:
      myHisto = new TH2F(name.c_str(), "h", 260, 0, 99, 100, 0, 99); // 100kB
      break;
    case 500:
      myHisto = new TH2F(name.c_str(), "h", 1250, 0, 99, 100, 0, 99); // 500kB
      break;
    case 1000:
      myHisto = new TH2F(name.c_str(), "h", 2500, 0, 99, 100, 0, 99); // 1MB
      break;
    case 2500:
      myHisto = new TH2F(name.c_str(), "h", 6250, 0, 99, 100, 0, 99); // 2.5MB
      break;
    case 5000:
      myHisto = new TH2F(name.c_str(), "h", 12500, 0, 99, 100, 0, 99); // 5MB
      break;
    default:
      BOOST_THROW_EXCEPTION(
        FatalException() << errinfo_details(
          "size of histo must be 1, 10, 100, 500, 1000, 2500 or 5000 (was: " + to_string(mSizeObjects) + ")"));
  }
  return myHisto;
}

void RepositoryBenchmark::InitTask()
{
  // parse arguments database
  string dbUrl = fConfig->GetValue<string>("database-url");
  string dbBackend = fConfig->GetValue<string>("database-backend");
  mTaskName = fConfig->GetValue<string>("task-name");
  try {
    mDatabase = o2::quality_control::repository::DatabaseFactory::create(dbBackend);
    mDatabase->connect(fConfig->GetValue<string>("database-url"), fConfig->GetValue<string>("database-name"),
                       fConfig->GetValue<string>("database-username"), fConfig->GetValue<string>("database-password"));
    mDatabase->prepareTaskDataContainer(mTaskName);
  } catch (boost::exception& exc) {
    string diagnostic = boost::current_exception_diagnostic_information();
    std::cerr << "Unexpected exception, diagnostic information follows:\n"
              << diagnostic << endl;
    if (diagnostic == "No diagnostic information available.") {
      throw;
    }
  }

  // parse other arguments
  mMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
  mNumberObjects = fConfig->GetValue<uint64_t>("number-objects");
  mSizeObjects = fConfig->GetValue<uint64_t>("size-objects");
  mDeletionMode = static_cast<bool>(fConfig->GetValue<int>("delete"));
  mObjectName = fConfig->GetValue<string>("object-name");
  auto numberTasks = fConfig->GetValue<uint64_t>("number-tasks");

  // monitoring
  mMonitoring = MonitoringFactory::Get(fConfig->GetValue<string>("monitoring-url"));
  mThreadedMonitoring = static_cast<bool>(fConfig->GetValue<int>("monitoring-threaded"));
  mThreadedMonitoringInterval = fConfig->GetValue<int>("monitoring-threaded-interval");
  mMonitoring->enableProcessMonitoring(1); // collect every seconds metrics for this process
  mMonitoring->addGlobalTag("taskName", mTaskName);
  mMonitoring->addGlobalTag("numberObject", to_string(mNumberObjects));
  mMonitoring->addGlobalTag("sizeObject", to_string(mSizeObjects));
  if (mTaskName == "benchmarkTask_0") { // send these parameters to monitoring only once per benchmark run
    mMonitoring->sendGrouped("ccdb-benchmark-parameters", { { mNumberObjects, "number-objects" },
                                                            { mSizeObjects * 1000, "size-objects" },
                                                            { numberTasks, "number-tasks" } });
  }

  if (mDeletionMode) {
    QcInfoLogger::GetInstance() << "Deletion mode..." << infologger::endm;
    emptyDatabase();
  }

  // prepare objects
  for (uint64_t i = 0; i < mNumberObjects; i++) {
    TH1* histo = createHisto(mSizeObjects, mObjectName + to_string(i));
    shared_ptr<MonitorObject> mo = make_shared<MonitorObject>(histo, mTaskName);
    mo->setIsOwner(true);
    mMyObjects.push_back(mo);
  }

  // start a timer in a thread to send monitoring metrics, if needed
  if (mThreadedMonitoring) {
    mTimer = new boost::asio::deadline_timer(io, boost::posix_time::seconds(mThreadedMonitoringInterval));
    mTimer->async_wait(boost::bind(&RepositoryBenchmark::checkTimedOut, this));
    th = new thread([&] { io.run(); });
  }
}

void RepositoryBenchmark::checkTimedOut()
{
  mMonitoring->send({ mTotalNumberObjects, "objectsSent" }, DerivedMetricMode::RATE);

  // restart timer
  mTimer->expires_at(mTimer->expires_at() + boost::posix_time::seconds(mThreadedMonitoringInterval));
  mTimer->async_wait(boost::bind(&RepositoryBenchmark::checkTimedOut, this));
}

bool RepositoryBenchmark::ConditionalRun()
{
  if (mDeletionMode) { // the only way to not run is to return false from here.
    return false;
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  // Store the object
  for (unsigned int i = 0; i < mNumberObjects; i++) {
    mDatabase->store(mMyObjects[i]);
    mTotalNumberObjects++;
  }
  if (!mThreadedMonitoring) {
    mMonitoring->send({ mTotalNumberObjects, "objectsSent" }, DerivedMetricMode::RATE);
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  long duration = duration_cast<milliseconds>(t2 - t1).count();
  mMonitoring->send({ duration / mNumberObjects, "storeDurationForOneObject_ms" }, DerivedMetricMode::NONE);

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

void RepositoryBenchmark::emptyDatabase()
{
  mDatabase->truncate(mTaskName, mObjectName);
  for (uint64_t i = 0; i < mNumberObjects; i++) {
    mDatabase->truncate(mTaskName, mObjectName + to_string(i));
  }
}

} // namespace o2::quality_control::core