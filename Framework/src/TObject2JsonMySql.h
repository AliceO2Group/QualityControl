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

#ifndef QUALITYCONTROL_TOBJECT2JSON_MYSQL_H
#define QUALITYCONTROL_TOBJECT2JSON_MYSQL_H

#include "TObject2JsonBackend.h"

// QualityControl
#include "QualityControl/MySqlDatabase.h"

using o2::quality_control::repository::MySqlDatabase;

namespace o2 {
namespace quality_control {
namespace tobject_to_json {
namespace backends {

/// Converts ROOT objects into JSON format, readable by JSROOT
class MySql final : public Backend
{
  public:
    // Connects to MySQL database
    MySql(std::string host, unsigned int port, std::string database, std::string username, std::string password);

    /// Gets TObject from database and returns the JSON equivalent
    std::string getJsonObject(std::string agentName, std::string objectName) override;
  
  private:
    /// \brief MySQL client instance from QualityControl framework
    std::unique_ptr<MySqlDatabase> mSqlClient;
};

} // namespace backends {
} // namespace tobject_to_json
} // namespace quality_control
} // namespace o2

#endif // QUALITYCONTROL_TOBJECT2JSON_MYSQL_H
