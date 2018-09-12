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
/// \file   TObejct2JsonServer.h
/// \author Vladimir Kosmala
/// \author Adam Wegrzynek
///

#ifndef QC_TOBJECT2JSON_SERVER_H
#define QC_TOBJECT2JSON_SERVER_H

// std
#include <sstream>

#include <thread>

#include "TObject2JsonBackend.h"
#include "zmq.h"

using o2::quality_control::repository::MySqlDatabase;

namespace o2 {
namespace quality_control {
namespace tobject_to_json {

class TObject2JsonServer
{
  public:
    TObject2JsonServer();

    /// Prepare and start all threads (server and workers)
    void start(std::string backend, std::string zeromq, uint8_t numThreads);

    /// Thread function of server
    void run();

  private:
    /// ZeroMQ context for server and backend sockets
    zmq::context_t mCtx;

    /// ZeroMQ server socket
    zmq::socket_t mFrontend;

    /// ZeroMQ backend in-process socket
    zmq::socket_t mBackend;
};

} // namespace tobject_to_json {
} // namespace quality_control
} // namespace o2

#endif // QC_TOBJECT2JSON_SERVER_H
