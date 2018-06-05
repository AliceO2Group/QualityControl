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

#include "FairMQLogger.h"
#include "options/FairMQProgOptions.h" // device->fConfig

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseFactory.h"
#include "Common/Exceptions.h"

using namespace std;
using namespace std::chrono;
using namespace AliceO2::Common;
using namespace o2::quality_control::repository;

namespace o2 {
namespace quality_control {
namespace core {

CcdbBenchmark::CcdbBenchmark()
  : mMaxIterations(0), mNumIterations(0), mSizeObjects(1), mNumberObjects(1)
{
}

void CcdbBenchmark::InitTask()
{
  try {
    string db = fConfig->GetValue<string>("ccdb-url");
    mDatabase = dynamic_cast<CcdbDatabase*>(o2::quality_control::repository::DatabaseFactory::create("CCDB"));
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

  if (mDeletionMode) {
    QcInfoLogger::GetInstance() << "Deletion mode..." << infologger::endm;
    emptyDatabase();
  }

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
    case 1000:
      mMyHisto = new TH2F("h", "h", 2500, 0, 99, 100, 0, 99); // 1MB
      for (unsigned int i = 0; i < 1000; i++) {
        mMyHisto->Fill(i);
      }
      break;
    default:
      BOOST_THROW_EXCEPTION(
        FatalException() << errinfo_details("size of histo must be 1, 10, 100 or 1000, not " + mSizeObjects));
  }
  mMyObject = new MonitorObject(mObjectName, mMyHisto, mTaskName);
}

bool CcdbBenchmark::ConditionalRun()
{
  if (mDeletionMode) { //the only way to not run is to return false from here.
    return false;
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  for(int i = 0 ; i < mNumberObjects ; i++) {
    mDatabase->store(mMyObject);
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();

  auto duration = duration_cast<microseconds>(t2 - t1).count();

  QcInfoLogger::GetInstance() << "Execution duration : " << duration << " us" << infologger::endm;

  auto duration2 = duration_cast<microseconds>(t2 - t1);
  auto remaining = duration_cast<microseconds>(std::chrono::seconds(1) - duration2);
//  QcInfoLogger::GetInstance() << "Remaining duration : " << remaining.count() << " us" << infologger::endm;

  if(remaining.count() < 0) {
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

CcdbBenchmark::~CcdbBenchmark()
{
}

void CcdbBenchmark::emptyDatabase()
{
  mDatabase->deleteAllObjectVersions(mTaskName, mObjectName);
}

}
}
}