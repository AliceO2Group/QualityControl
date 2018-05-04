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
/// \file   TObject2JsonFactory.cxx
/// \author Adam Wegrzynek
///

#include "TObject2JsonFactory.h"
#include "TObject2Json.h"
#include "TObject2JsonMySql.h"
#include "TObject2JsonCcdb.h"
#include "external/UriParser.h"

namespace o2 {
namespace quality_control {
namespace tobject_to_json {


auto getMySql(const http::url& uri) {
  int port = (uri.port == 0) ? 3306 : uri.port;
  std::string database = uri.path;
  database.erase(database.begin(), database.begin() + 1);
  return std::make_unique<backends::MySql>(uri.host, port, database, uri.user, uri.password);
}

auto getCcdb(const http::url& uri) {
  int port = (uri.port == 0) ? 3306 : uri.port;
  std::string database = uri.path;
  database.erase(database.begin(), database.begin() + 1);
  return std::make_unique<backends::Ccdb>(uri.host, port, database, uri.user, uri.password);
}


std::unique_ptr<TObject2Json> TObject2JsonFactory::Get(std::string url, std::string zeromqUrl) {
  static const std::map<std::string, std::function<std::unique_ptr<Backend>(const http::url&)>> map = {
      {"mysql", getMySql},
      {"ccdb", getCcdb}
  };

  http::url parsedUrl = http::ParseHttpUrl(url);
  if (parsedUrl.protocol.empty()) {
    throw std::runtime_error("Ill-formed URI");
  }
  auto iterator = map.find(parsedUrl.protocol);
  if (iterator != map.end()) {
    return std::make_unique<TObject2Json>(iterator->second(parsedUrl), zeromqUrl);
  } else {
    throw std::runtime_error("Unrecognized backend " + parsedUrl.protocol);
  }
}

} // namespace tobject_to_json
} // namespace quality_control
} // namespace o2
