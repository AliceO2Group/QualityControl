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
#include "QualityControl/Version.h"
#include "QualityControl/QcInfoLogger.h"
#include "Common/Exceptions.h"
// ROOT
#include <TBufferJSON.h>
#include <TH1F.h>
#include <TFile.h>
#include <TList.h>
#include <TROOT.h>
#include <TKey.h>
#include <TStreamerInfo.h>
#include <TSystem.h>
// std
#include <chrono>
#include <sstream>
#include <unordered_set>

#include <boost/algorithm/string.hpp>

using namespace std::chrono;
using namespace AliceO2::Common;
using namespace o2::quality_control::core;
using namespace std;

namespace o2::quality_control::repository
{

CcdbDatabase::~CcdbDatabase() { disconnect(); }

void CcdbDatabase::loadDeprecatedStreamerInfos()
{
  if (getenv("QUALITYCONTROL_ROOT") == nullptr) {
    ILOG(Warning) << "QUALITYCONTROL_ROOT is not set thus the the streamerinfo ROOT file can't be found.\n"
                  << "Consequently, old data might not be readable." << ENDM;
    return;
  }
  string path = string(getenv("QUALITYCONTROL_ROOT")) + "/etc/";
  vector<string> filenames = { "streamerinfos.root", "streamerinfos_v017.root" };
  for (auto filename : filenames) {
    string localPath = path + filename;
    ILOG(Info) << "Loading streamerinfos from : " << localPath << ENDM;
    TFile file(localPath.data(), "READ");
    if (file.IsZombie()) {
      string s = string("Cannot find ") + localPath;
      ILOG(Error) << s << ENDM;
      BOOST_THROW_EXCEPTION(DatabaseException() << errinfo_details(s));
    }
    TIter next(file.GetListOfKeys());
    TKey* key;
    std::unordered_set<std::string> alreadySeen;
    while ((key = (TKey*)next())) {
      TClass* cl = gROOT->GetClass(key->GetClassName());
      if (!cl->InheritsFrom("TStreamerInfo"))
        continue;
      auto* si = (TStreamerInfo*)key->ReadObj();
      string stringRepresentation = si->GetName() + si->GetClassVersion();
      if (alreadySeen.count(stringRepresentation) == 0) {
        alreadySeen.emplace(stringRepresentation);
        ILOG(Debug) << "importing streamer info version " << si->GetClassVersion() << " for '" << si->GetName() << ENDM;
        si->BuildCheck();
      }
    }
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

// Monitor object
void CcdbDatabase::storeMO(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  if (mo->getName().length() == 0 || mo->getTaskName().length() == 0) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't be empty. Do not store. "));
  }

  if (mo->getName().find_first_of("\t\n ") != string::npos || mo->getTaskName().find_first_of("\t\n ") != string::npos) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't contain white spaces. Do not store."));
  }

  // metadata
  map<string, string> metadata;
  // QC metadata (prefix qc_)
  metadata["qc_version"] = Version::GetQcVersion().getString();
  // user metadata
  map<string, string> userMetadata = mo->getMetadataMap();
  if (!userMetadata.empty()) {
    metadata.insert(userMetadata.begin(), userMetadata.end());
  }

  // other attributes
  string path = mo->getPath();
  long from = getCurrentTimestamp();
  long to = getFutureTimestamp(60 * 60 * 24 * 365 * 10);

  // extract object and metadata from MonitorObject
  TObject* obj = mo->getObject();
  metadata["qc_detector_name"] = mo->getDetectorName();
  metadata["qc_task_name"] = mo->getTaskName();
  metadata["ObjectType"] = mo->getObject()->IsA()->GetName(); // ObjectType says TObject and not MonitorObject due to a quirk in the API. Once fixed, remove this.

  ccdbApi.storeAsTFileAny<TObject>(obj, path, metadata, from, to);
}

void CcdbDatabase::storeQO(std::shared_ptr<QualityObject> qo)
{
  // metadata
  map<string, string> metadata;
  // QC metadata (prefix qc_)
  metadata["qc_version"] = Version::GetQcVersion().getString();
  metadata["qc_quality"] = std::to_string(qo->getQuality().getLevel());

  // other attributes
  string path = qo->getPath();
  long from = getCurrentTimestamp();
  long to = getFutureTimestamp(60 * 60 * 24 * 365 * 10);

  ccdbApi.storeAsTFileAny<QualityObject>(qo.get(), path, metadata, from, to);
}

TObject* CcdbDatabase::retrieveTObject(std::string path, long timestamp, std::map<std::string, std::string>* headers)
{
  map<string, string> metadata;
  long when = timestamp == 0 ? getCurrentTimestamp() : timestamp;

  // we try first to load a TFile
  auto* object = ccdbApi.retrieveFromTFileAny<TObject>(path, metadata, when, headers);
  if (object == nullptr) {
    // We could not open a TFile we should now try to open an object directly serialized
    object = ccdbApi.retrieve(path, metadata, when);
    if (object == nullptr) {
      ILOG(Error) << "We could NOT retrieve the object " << path << "." << ENDM;
      return nullptr;
    }
  }
  ILOG(Debug) << "Retrieved object " << path << ENDM;
  return object;
}

std::shared_ptr<core::MonitorObject> CcdbDatabase::retrieveMO(std::string taskName, std::string objectName, long timestamp)
{
  string path = taskName + "/" + objectName;
  long when = timestamp == 0 ? getCurrentTimestamp() : timestamp;
  map<string, string> headers;
  TObject* obj = retrieveTObject(path, when, &headers);
  Version objectVersion(headers["qc_version"]); // retrieve headers to determine the version of the QC framework

  std::shared_ptr<MonitorObject> mo;
  if (objectVersion == Version("0.0.0") || objectVersion < Version("0.25")) {
    ILOG(Debug) << "Version of object " << taskName << "/" << objectName << " is < 0.25" << ENDM;
    // The object is either in a TFile or is a blob but it was stored with storeAsTFile as a full MO
    mo.reset(dynamic_cast<MonitorObject*>(obj));
    if (mo == nullptr) {
      ILOG(Error) << "Could not cast the object " << taskName << "/" << objectName << " to MonitorObject" << ENDM;
    }
  } else {
    // Version >= 0.25 -> the object is stored directly unencapsulated
    ILOG(Debug) << "Version of object " << taskName << "/" << objectName << " is >= 0.25" << ENDM;
    mo = make_shared<MonitorObject>(obj, headers["qc_task_name"], headers["qc_detector_name"]);
    // TODO should we remove the headers we know are general such as ETag and qc_task_name ?
    mo->addMetadata(headers);
  }
  return mo;
}

std::shared_ptr<QualityObject> CcdbDatabase::retrieveQO(std::string qoPath, long timestamp)
{
  long when = timestamp == 0 ? getCurrentTimestamp() : timestamp;
  TObject* obj = retrieveTObject(qoPath, when);
  std::shared_ptr<QualityObject> qo(dynamic_cast<QualityObject*>(obj));
  if (qo == nullptr) {
    ILOG(Error) << "Could not cast the object " << qoPath << " to QualityObject" << ENDM;
  }
  return qo;
}

std::string CcdbDatabase::retrieveQOJson(std::string qoPath, long timestamp)
{
  return retrieveJson(qoPath, timestamp);
}

std::string CcdbDatabase::retrieveMOJson(std::string taskName, std::string objectName, long timestamp)
{
  string path = taskName + "/" + objectName;
  return retrieveJson(path, timestamp);
}

std::string CcdbDatabase::retrieveJson(std::string path, long timestamp)
{
  map<string, string> headers;
  auto tobj = retrieveTObject(path, timestamp, &headers);

  if (tobj == nullptr) {
    return std::string();
  }

  TObject* toConvert = nullptr;
  if (tobj->IsA() == MonitorObject::Class()) { // a full MO -> pre-v0.25
    std::shared_ptr<MonitorObject> mo(dynamic_cast<MonitorObject*>(tobj));
    toConvert = mo->getObject();
    mo->setIsOwner(false);
  } else if (tobj->IsA() == QualityObject::Class()) {
    toConvert = dynamic_cast<QualityObject*>(tobj);
  } else { // something else but a TObject
    toConvert = tobj;
  }
  if (toConvert == nullptr) {
    ILOG(Error) << "Unable to get the object to convert" << ENDM;
    return std::string();
  }
  TString json = TBufferJSON::ConvertToJSON(toConvert);
  delete toConvert;

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

std::string CcdbDatabase::getListingAsString(std::string subpath, std::string accept)
{
  std::string tempString = ccdbApi.list(subpath, false, accept);

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

std::vector<std::string> CcdbDatabase::getListing(std::string subpath)
{
  std::vector<string> result;

  // Get the listing from CCDB (folder qc)
  string listing = getListingAsString(subpath);

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
  std::string taskNameEscaped = boost::replace_all_copy(taskName, "/", "\\/");
  while (std::getline(ss, line, '\n')) {
    ltrim(line);
    rtrim(line);
    if (line.length() > 0 && line.find("\"path\"") == 0) {
      unsigned long objNameStart = 9 + taskNameEscaped.length();
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
  ILOG(Info) << "truncating data for " << taskName << "/" << objectName << ENDM;

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
