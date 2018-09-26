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
/// \file   TObejct2JsonWorker.h
/// \author Vladimir Kosmala
///

#ifndef QC_TOBJECT2JSON_WORKER_H
#define QC_TOBJECT2JSON_WORKER_H

#include <thread>

#include "TObject2JsonBackend.h"
#include "zmq.h"

namespace o2 {
namespace quality_control {
namespace tobject_to_json {

/*! \brief Translates ROOT objects to JSON inside a thread from backend given
 *         by responding to requests sent via ZeroMQ
 */
class TObject2JsonWorker
{
  public:
    /// Create a thread and listen for requests
    TObject2JsonWorker(zmq::context_t &ctx, std::unique_ptr<Backend> backend);

    /// Stops listening for requests and stop thread
    ~TObject2JsonWorker();

    /// Start listening to requets
    void start();

    /// Handle ZeroMQ request
    std::string handleRequest(std::string request);

    ///  Receive 0MQ string from socket and convert into string
    std::string socketReceive();

    ///  Sends string as 0MQ string, as multipart non-terminal
    bool socketSend(const std::string & identity, const std::string & payload);

    std::string response200(std::string request, std::string payload);
    std::string responseError(int code, std::string request, std::string error = {});

  private:
    /// Backend instance (MySQL or CCDB)
    std::unique_ptr<Backend> mBackend;

    /// ZeroMQ context
    zmq::context_t &mCtx;

    /// ZeroMQ client socket
    zmq::socket_t mWorkerSocket;

    /// Thread
    std::thread mThread;

    /// Flag to continue listening requests or not and exit
    bool mRun;
};

} // namespace tobject_to_json {
} // namespace quality_control
} // namespace o2

#endif // QC_TOBJECT2JSON_WORKER_H
