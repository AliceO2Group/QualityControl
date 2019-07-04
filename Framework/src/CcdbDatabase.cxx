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
#include "QualityControl/MonitorObject.h"
#include "Common/Exceptions.h"
// ROOT
#include <TBufferJSON.h>
#include <TH1F.h>
#include <TFile.h>
#include <TList.h>
#include <TROOT.h>
#include <TFile.h>
#include <TKey.h>
#include <TStreamerInfo.h>
// boost
#include <boost/algorithm/string.hpp>
// std
#include <chrono>
#include <sstream>
#include <utility>
#include <fairlogger/Logger.h>

#include <TSystem.h>

using namespace std::chrono;
using namespace AliceO2::Common;
using namespace o2::quality_control::core;
using namespace std;

namespace o2::quality_control::repository
{

CcdbDatabase::CcdbDatabase() : mUrl("")
{
}

CcdbDatabase::~CcdbDatabase() { disconnect(); }

void CcdbDatabase::loadDeprecatedStreamerInfos()
{
  if (getenv("QUALITYCONTROL_ROOT") == nullptr) {
    LOG(WARNING) << "QUALITYCONTROL_ROOT is not set thus the the streamerinfo ROOT file can't be found.\n"
                 << "Consequently, old data might not be readable.";
    return;
  }
  string path = string(getenv("QUALITYCONTROL_ROOT")) + "/etc/";
  path += "streamerinfos.root";
  TFile file(path.data(), "READ");
  if (file.IsZombie()) {
    string s = string("Cannot find ") + path;
    LOG(ERROR) << s;
    BOOST_THROW_EXCEPTION(DatabaseException() << errinfo_details(s));
  }
  TIter next(file.GetListOfKeys());
  TKey* key;
  while ((key = (TKey*)next())) {
    TClass* cl = gROOT->GetClass(key->GetClassName());
    if (!cl->InheritsFrom("TStreamerInfo"))
      continue;
    auto* si = (TStreamerInfo*)key->ReadObj();
    LOG(DEBUG) << "importing streamer info version " << si->GetClassVersion() << " for '" << si->GetName();
    si->BuildCheck();
  }
}

void CcdbDatabase::connect(std::string host, std::string /*database*/, std::string /*username*/, std::string /*password*/)
{
  mUrl = host;
  init();
}

void CcdbDatabase::connect(const std::unordered_map<std::string, std::string>& config)
{
  mUrl = config.at("host");
  init();
}

void CcdbDatabase::init()
{
  ccdbApi.init(mUrl);
  loadDeprecatedStreamerInfos();
}

void CcdbDatabase::store(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  if (mo->getName().length() == 0 || mo->getTaskName().length() == 0) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't be empty. Do not store."));
  }

  if (mo->getName().find_first_of("\t\n ") != string::npos || mo->getTaskName().find_first_of("\t\n ") != string::npos) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't contain white spaces. Do not store."));
  }

  // metadata
  map<string, string> metadata;
  metadata["quality"] = std::to_string(mo->getQuality().getLevel());
  map<string, string> userMetadata = mo->getMetadataMap();
  if (!userMetadata.empty()) {
    metadata.insert(userMetadata.begin(), userMetadata.end());
  }

  // other attributes
  string path = mo->getTaskName() + "/" + mo->getName();
  long from = getCurrentTimestamp();
  long to = getFutureTimestamp(60 * 60 * 24 * 365 * 10);

  ccdbApi.store(mo.get(), path, metadata, from, to);
}

core::MonitorObject* CcdbDatabase::retrieve(std::string taskName, std::string objectName, long timestamp)
{
  string path = taskName + "/" + objectName;
  map<string, string> metadata;
  long when = timestamp == 0 ? getCurrentTimestamp() : timestamp;

  // we try first to load a TFile
  TObject* object = ccdbApi.retrieveFromTFile(path, metadata, when);
  if (object == nullptr) {
    // We could not open a TFile we should now try to open an object directly serialized
    object = ccdbApi.retrieve(path, metadata, when);
    LOG(DEBUG) << "We could retrieve the object " << path << " as a streamed object.";
    if (object == nullptr) {
      return nullptr;
    }
  }
  auto* mo = dynamic_cast<core::MonitorObject*>(object);
  if (mo == nullptr) {
    LOG(ERROR) << "Could not cast the object " << taskName << "/" << objectName << " to MonitorObject";
  }
  return mo;
}

std::string CcdbDatabase::retrieveJson(std::string taskName, std::string objectName)
{
  std::unique_ptr<core::MonitorObject> monitor(retrieve(taskName, objectName));
  if (monitor == nullptr) {
    return std::string();
  }
  std::unique_ptr<TObject> obj(monitor->getObject());
  monitor->setIsOwner(false);
  TString json = TBufferJSON::ConvertToJSON(obj.get());
  return json.Data();
}

void CcdbDatabase::disconnect()
{
  // NOOP for CCDB
}

void CcdbDatabase::prepareTaskDataContainer(std::string /*taskName*/)
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
  LOG(INFO) << "truncating data for " << taskName << "/" << objectName;

  ccdbApi.truncate(taskName + "/" + objectName);
}

void CcdbDatabase::storeStreamerInfosToFile(std::string filename)
{
  TH1F* h1 = new TH1F("asdf", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "fake");
  TMessage message(kMESS_OBJECT);
  message.Reset();
  message.EnableSchemaEvolution(true);
  message.WriteObjectAny(mo1.get(), mo1->IsA());
  TList* infos = message.GetStreamerInfos();
  TFile f(filename.data(), "recreate");
  infos->Write();
  f.Close();
}

} // namespace o2::quality_control::repository
