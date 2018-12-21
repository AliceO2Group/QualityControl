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
/// \file   CcdbDatabase.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/CcdbDatabase.h"
#include "Common/Exceptions.h"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <sstream>

using namespace std::chrono;
using namespace AliceO2::Common;

namespace o2
{
namespace quality_control
{
namespace repository
{

using namespace std;

CcdbDatabase::CcdbDatabase() : mUrl("") {}

CcdbDatabase::~CcdbDatabase() { disconnect(); }

void CcdbDatabase::connect(std::string host, std::string database, std::string username, std::string password)
{
  mUrl = host;
  ccdbApi.init(mUrl);
}

void CcdbDatabase::connect(const std::unordered_map<std::string, std::string>& config)
{
  mUrl = config.at("host");
  ccdbApi.init(mUrl);
}

void CcdbDatabase::store(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  if (mo->getName().length() == 0 || mo->getTaskName().length() == 0) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't be empty. Do not store."));
  }

  string path = mo->getTaskName() + "/" + mo->getName();
  map<string, string> metadata;
  metadata["quality"] = std::to_string(mo->getQuality().getLevel());
  long from = getCurrentTimestamp();
  long to = getFutureTimestamp(60 * 60 * 24 * 365 * 10); // todo set a proper timestamp for the end

  cout << "storing : " << path << endl;
  ccdbApi.store(mo.get(), path, metadata, from, to);
}

/**
 * Struct to store the data we will receive from the CCDB with CURL.
 */
struct MemoryStruct {
  char* memory;
  unsigned int size;
};

core::MonitorObject* CcdbDatabase::retrieve(std::string taskName, std::string objectName)
{

  string path = taskName + "/" + objectName;
  map<string, string> metadata;

  TObject* object = ccdbApi.retrieve(path, metadata, getCurrentTimestamp());
  return dynamic_cast<core::MonitorObject*>(object);
}

void CcdbDatabase::disconnect()
{
  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();
}

void CcdbDatabase::prepareTaskDataContainer(std::string taskName)
{
  // NOOP for CCDB
}

std::string CcdbDatabase::getListing(std::string path, std::string accept)
{
  std::string tempString = ccdbApi.list(path, false, accept);

  return tempString;
}

/// trim from start (in place)
/// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline void ltrim(std::string& s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

/// trim from end (in place)
/// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
static inline void rtrim(std::string& s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

std::vector<std::string> CcdbDatabase::getListOfTasksWithPublications()
{
  std::vector<string> result;

  // Get the listing from CCDB
  string listing = getListing();

  // Split the string we received, by line. Also trim it and remove empty lines.
  std::stringstream ss(listing);
  std::string line;
  while (std::getline(ss, line, '\n')) {
    ltrim(line);
    rtrim(line);
    if (line.length() > 0 && line != "Subfolders:") {
      result.push_back(line);
    }
  }

  return result;
}

std::vector<std::string> CcdbDatabase::getPublishedObjectNames(std::string taskName)
{
  std::vector<string> result;

  string listing = ccdbApi.list(taskName + "/.*", true, "Application/JSON");

  // Split the string we received, by line. Also trim it and remove empty lines. Select the lines starting with "path".
  std::stringstream ss(listing);
  std::string line;
  while (std::getline(ss, line, '\n')) {
    ltrim(line);
    rtrim(line);
    if (line.length() > 0 && line.find("\"path\"") == 0) {
      unsigned long objNameStart = 9 + taskName.length();
      string path = line.substr(objNameStart, line.length() - 2 /*final 2 char*/ - objNameStart);
      result.push_back(path);
    }
  }

  return result;
}

long CcdbDatabase::getFutureTimestamp(int secondsInFuture)
{
  std::chrono::seconds sec(secondsInFuture);
  auto future = std::chrono::system_clock::now() + sec;
  auto future_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(future);
  auto epoch = future_ms.time_since_epoch();
  auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
  return value.count();
}

long CcdbDatabase::getCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto epoch = now_ms.time_since_epoch();
  auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
  return value.count();
}

void CcdbDatabase::truncate(std::string taskName, std::string objectName)
{
  cout << "truncating data for " << taskName << "/" << objectName << endl;

  ccdbApi.truncate(taskName + "/" + objectName);
}

} // namespace repository
} // namespace quality_control
} // namespace o2
