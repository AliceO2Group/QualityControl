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
/// \file   TObejct2JsonCcdb.h
/// \author Barthelemy von Haller
/// \author Adam Wegrzynek
///

#ifndef QUALITYCONTROL_TOBJECT2JSON_CCDB_H
#define QUALITYCONTROL_TOBJECT2JSON_CCDB_H

#include "TObject2JsonBackend.h"

// QualityControl
#include "QualityControl/CcdbDatabase.h"

using o2::quality_control::repository::CcdbDatabase;

namespace o2 {
namespace quality_control {
namespace tobject_to_json {
namespace backends {

/// Takes TObject from Ccdb database and returns JSON formatted object
class Ccdb final : public Backend
{
  public:
    // Connects to MySQL database
    Ccdb(std::string host, unsigned int port, std::string database, std::string username, std::string password);

    /// Gets TObject from database and returns the JSON equivalent
    std::string getJsonObject(std::string agentName, std::string objectName) override;

  private:
    /// MySQL client instance
    std::unique_ptr<CcdbDatabase> mCcdbClient;
};

} // namespace backends {
} // namespace tobject_to_json
} // namespace quality_control
} // namespace o2

#endif // QUALITYCONTROL_TOBJECT2JSON_MYSQL_H
