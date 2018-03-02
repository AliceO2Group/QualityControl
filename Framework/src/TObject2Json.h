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
/// \file   TObejct2Json.h
/// \author Vladimir Kosmala
/// \author Adam Wegrzynek
///

#ifndef QUALITYCONTROL_TOBJECT2JSON_H
#define QUALITYCONTROL_TOBJECT2JSON_H

#include "TObject2JsonBackend.h"

using o2::quality_control::repository::MySqlDatabase;

namespace o2 {
namespace quality_control {
namespace tobject_to_json {

/// \brief Converts ROOT objects into JSON format, readable by JSROOT
class TObject2Json
{
  public:
    TObject2Json(std::unique_ptr<Backend> backend, std::string zeromqUrl);
    /// Listens on the the ZMQ server endpoint
    void start();

  private:
    /// MySQL client instance from QualityControl framework
    std::unique_ptr<Backend> mBackend;

    /// ZeroMQ context
    void *mContext;

    /// ZeroMQ server socket
    void *mSocket;

    // Handle ZeroMQ request
    std::string handleRequest(std::string message);
};

} // namespace tobject_to_json {
} // namespace quality_control
} // namespace o2

#endif // QUALITYCONTROL_TOBJECT2JSON_H
