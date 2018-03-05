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
/// \file   MySQL.cxx
/// \author Vladimir Kosmala
/// \author Adam Wegrzynek
///

#include "TObject2JsonMySql.h"
// QualityControl
#include "QualityControl/MySqlDatabase.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include "TBufferJSON.h"

using o2::quality_control::repository::MySqlDatabase;
using o2::quality_control::core::MonitorObject;
using o2::quality_control::core::QcInfoLogger;

namespace o2 {
namespace quality_control {
namespace tobject_to_json {
namespace backends {

MySql::MySql(std::string host, unsigned int port, std::string database, std::string username, std::string password)
{
  host += ":" + std::to_string(port);
  mSqlClient = std::make_unique<MySqlDatabase>();
  mSqlClient->connect(host, database, username, password);
  QcInfoLogger::GetInstance() << "MySQL backend created: " << host << "/" << database << infologger::endm;
}

std::string MySql::getJsonObject(std::string agentName, std::string objectName)
{
  std::unique_ptr<MonitorObject> monitor(mSqlClient->retrieve(agentName, objectName));
  std::unique_ptr<TObject> obj(monitor->getObject());
  monitor->setIsOwner(false);
  TString json = TBufferJSON::ConvertToJSON(obj.get());
  return json.Data();
}

} // namespace backends
} // namespace tobject_to_json
} // namespace quality_control
} // namespace o2
