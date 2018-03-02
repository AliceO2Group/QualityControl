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
/// \file   TObejct2Json.cxx
/// \author Vladimir Kosmala
/// \author Adam Wegrzynek
///

// TObject2Json
#include "TObject2Json.h"
#include "TObject2JsonMySql.h"
#include "QualityControl/QcInfoLogger.h"

#include <chrono>
#include <thread>

// ZMQ
#include <zmq.h>

// Boost
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace o2::quality_control::core;
using namespace std::string_literals;

namespace o2 {
namespace quality_control {
namespace tobject_to_json {

TObject2Json::TObject2Json(std::unique_ptr<Backend> backend, std::string zeromqUrl)
  : mBackend(std::move(backend))
{
  mContext = zmq_ctx_new ();
  mSocket = zmq_socket (mContext, ZMQ_REP);
  int rc = zmq_bind (mSocket, zeromqUrl.c_str());
  if (rc != 0) {
    throw std::runtime_error("Couldn't bind the socket "s + zmq_strerror(zmq_errno()));
  }
  QcInfoLogger::GetInstance() << "ZeroMQ server: Socket bound " << zeromqUrl << infologger::endm;
}

string TObject2Json::handleRequest(string request)
{
  if (request.length() == 0) {
    QcInfoLogger::GetInstance() << "Empty request received, ignoring..." << infologger::endm;
    return "";
  }

  // Split arguments with space
  vector<string> parts;
  boost::split(parts, request, boost::is_any_of(" "));
  
  if (parts.size() != 2) {
    QcInfoLogger::GetInstance() << "! Service requires 2 arguments" << infologger::endm;
  }
  
  string agentName = parts[0];
  string objectName = parts[1];
  try {
    return mBackend->getJsonObject(agentName, objectName);
  } catch (const std::exception& error) {
    return "500 unhandled error";
  }
}

void TObject2Json::start()
{
  while(1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Wait for next request
    zmq_msg_t messageReq;
    zmq_msg_init(&messageReq);
    int size = zmq_msg_recv(&messageReq, mSocket, 0);
    if (size == -1) {
      QcInfoLogger::GetInstance() << "Unable to read socket: " << zmq_strerror(zmq_errno()) << infologger::endm;
      zmq_msg_close(&messageReq);
      continue;
    }
    // Process message
    string request((const char*)zmq_msg_data(&messageReq), size);
    zmq_msg_close(&messageReq);
    QcInfoLogger::GetInstance() << "Received request (" << request << ")" << infologger::endm;
    string response = handleRequest(request);
    QcInfoLogger::GetInstance() << "Response generated" << infologger::endm;
    // Send back response
    zmq_msg_t messageRep;
    zmq_msg_init_size(&messageRep, response.size());
    memcpy(zmq_msg_data(&messageRep), response.data(), response.size());
    size = zmq_msg_send(&messageRep, mSocket, 0);
    if (size == -1) {
      QcInfoLogger::GetInstance() << "Unable to write socket: " << zmq_strerror(zmq_errno()) << infologger::endm;
    }
    zmq_msg_close(&messageRep);
  }
}

} // namespace tobject_to_json
} // namespace quality_control
} // namespace o2
