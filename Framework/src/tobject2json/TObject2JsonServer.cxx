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
/// \file   TObject2JsonServer.cxx
/// \author Vladimir Kosmala
/// \author Adam Wegrzynek
///

// TObject2Json
#include "TObject2JsonServer.h"
#include "QualityControl/QcInfoLogger.h"
#include "TObject2JsonBackendFactory.h"
#include "TObject2JsonWorker.h"

// ZMQ
#include "zmq.h"

// Boost
#include <boost/algorithm/string.hpp>

#include <TROOT.h>

using namespace std;
using namespace o2::quality_control::core;
using namespace std::string_literals;
using o2::quality_control::tobject_to_json::TObject2JsonBackendFactory;

namespace o2
{
namespace quality_control
{
namespace tobject_to_json
{

TObject2JsonServer::TObject2JsonServer() : mCtx(1), mFrontend(mCtx, ZMQ_ROUTER), mBackend(mCtx, ZMQ_DEALER)
{
  // Make ROOT aware that workers will do calls from threads
  ROOT::EnableThreadSafety();
}

void TObject2JsonServer::start(std::string backendUrl, std::string zeromq, uint8_t numThreads)
{
  if (numThreads <= 0) {
    throw std::runtime_error("Number of workers must be >= 1");
  }

  std::vector<std::unique_ptr<TObject2JsonWorker>> workers;

  QcInfoLogger::GetInstance() << "Deploying workers..." << infologger::endm;
  for (int i = 0; i < numThreads; ++i) {
    std::unique_ptr<Backend> backendInstance = TObject2JsonBackendFactory::Get(backendUrl);
    auto worker = std::make_unique<TObject2JsonWorker>(mCtx, std::move(backendInstance));
    workers.push_back(std::move(worker));
    QcInfoLogger::GetInstance() << "Worker " << i << " started" << infologger::endm;
  }

  QcInfoLogger::GetInstance() << "Starting ZeroMQ server..." << infologger::endm;
  mFrontend.bind(zeromq);
  mBackend.bind("inproc://backend");
  QcInfoLogger::GetInstance() << "Ready for incoming requests" << infologger::endm;

  // We start zmq proxy in a separe thread to let main thread continue to handle signals
  // (avoid "Interrupted system call" error)
  std::thread mThread(std::bind(&TObject2JsonServer::run, this));
  mThread.join();
}

void TObject2JsonServer::run()
{
  try {
    zmq::proxy(mFrontend, mBackend, nullptr);
  } catch (std::exception& e) {
    QcInfoLogger::GetInstance() << "Closing server/backend proxy: " << e.what() << infologger::endm;
  }
}

} // namespace tobject_to_json
} // namespace quality_control
} // namespace o2
