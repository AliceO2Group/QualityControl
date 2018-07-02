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
/// \file   CcdbBenchmark.h
/// \author Barthelemy von Haller
///

#ifndef PROJECT_CCDBBENCHMARK_H
#define PROJECT_CCDBBENCHMARK_H

#include <FairMQDevice.h>
#include "QualityControl/CcdbDatabase.h"
#include <TH1F.h>
#include <Monitoring/MonitoringFactory.h>
#include <boost/asio.hpp>

namespace o2 {
namespace quality_control {
namespace core {

class CcdbBenchmark : public FairMQDevice
{
  public:
    CcdbBenchmark();
    virtual ~CcdbBenchmark()= default;

  protected:
    virtual void InitTask();
    virtual bool ConditionalRun();
    void emptyDatabase();
    void checkTimedOut();

  private:
    uint64_t mMaxIterations;
    uint64_t mNumIterations;
    uint64_t mNumberObjects;
    uint64_t mSizeObjects;
    std::string mTaskName;
    std::string mObjectName;
    bool mThreadedMonitoring;

    std::unique_ptr<o2::monitoring::Monitoring> mMonitoring;
    uint64_t mTotalNumberObjects;

    bool mDeletionMode;
    o2::quality_control::repository::DatabaseInterface* mDatabase;
    std::vector<std::shared_ptr<MonitorObject>> mMyObjects;
    TH1 *mMyHisto;

    // variables for the timer
    boost::asio::deadline_timer *mTimer; /// the asynchronous timer to send monitoring data
    boost::asio::io_service io;
    std::thread *th;
};

}
}
}

#endif //PROJECT_CCDBBENCHMARK_H
