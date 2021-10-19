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
/// \file   RepositoryBenchmark.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOSITORYBENCHMARK_H
#define QC_REPOSITORYBENCHMARK_H

#include "QualityControl/DatabaseInterface.h"
#include <fairmq/FairMQDevice.h>
#include <TH1.h>
#include <Monitoring/MonitoringFactory.h>
#include <boost/asio.hpp>
#include <thread>
#include <string>

namespace o2::quality_control::core
{

class RepositoryBenchmark : public FairMQDevice
{
 public:
  RepositoryBenchmark() = default;
  virtual ~RepositoryBenchmark() = default;

 protected:
  virtual void InitTask();
  virtual bool ConditionalRun();
  void emptyDatabase();
  void checkTimedOut();
  TH1* createHisto(uint64_t sizeObjects, std::string name);

 private:
  // user params
  uint64_t mMaxIterations = 0;
  uint64_t mNumIterations = 0;
  uint64_t mNumberObjects = 1;
  uint64_t mSizeObjects = 1;
  std::string mTaskName;
  std::string mObjectName;
  bool mDeletionMode = false; // todo: is false ok as default?

  // monitoring
  std::unique_ptr<o2::monitoring::Monitoring> mMonitoring;
  uint64_t mTotalNumberObjects = 0;
  bool mThreadedMonitoring = true;
  uint64_t mThreadedMonitoringInterval = 10;

  // internal state
  std::unique_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
  std::vector<std::shared_ptr<MonitorObject>> mMyObjects;
  //  TH1* mMyHisto;

  // variables for the timer
  boost::asio::deadline_timer* mTimer; /// the asynchronous timer to send monitoring data
  boost::asio::io_service io;
  std::thread* th;
};

} // namespace o2::quality_control::core

#endif // QC_REPOSITORYBENCHMARK_H
