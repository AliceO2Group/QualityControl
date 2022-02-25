// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
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
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/DatabaseHelpers.h"

#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>
#include <Common/Exceptions.h>
// O2
#include <CommonUtils/MemFileHelper.h>
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
#include <filesystem>
#include <unordered_set>
// boost
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
// misc
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace std::chrono;
using namespace AliceO2::Common;
using namespace o2::quality_control::core;
using namespace std;
using namespace rapidjson;

namespace o2::quality_control::repository
{

CcdbDatabase::~CcdbDatabase() { disconnect(); }

void CcdbDatabase::loadDeprecatedStreamerInfos()
{
  if (getenv("QUALITYCONTROL_ROOT") == nullptr) {
    ILOG(Warning, Support) << "QUALITYCONTROL_ROOT is not set thus the the streamerinfo ROOT file can't be found.\n"
                           << "Consequently, old data might not be readable." << ENDM;
    return;
  }
  string path = string(getenv("QUALITYCONTROL_ROOT")) + "/etc/";
  vector<string> filenames = { "streamerinfos.root", "streamerinfos_v017.root" };
  for (auto filename : filenames) {
    string localPath = path + filename;
    ILOG(Info, Devel) << "Loading streamerinfos from : " << localPath << ENDM;
    TFile file(localPath.data(), "READ");
    if (file.IsZombie()) {
      string s = string("Cannot find ") + localPath;
      ILOG(Error, Support) << s << ENDM;
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
        ILOG(Debug, Trace) << "importing streamer info version " << si->GetClassVersion() << " for '" << si->GetName() << "'" << ENDM;
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
  if (config.count("maxObjectSize")) {
    mMaxObjectSize = std::stoi(config.at("maxObjectSize"));
  }
}

void CcdbDatabase::init()
{
  ccdbApi.init(mUrl);
  loadDeprecatedStreamerInfos();
}

void CcdbDatabase::storeAny(const void* obj, std::type_info const& typeInfo, std::string const& path, std::map<std::string, std::string> const& metadata,
                            std::string const& detectorName, std::string const& taskName, long from, long to)
{
  if (obj == nullptr) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Cannot store a null pointer."));
  }
  if (path.length() == 0) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't be empty. Do not store."));
  }
  if (path.find_first_of("\t\n ") != string::npos) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't contain white spaces. Do not store."));
  }

  // metadata
  map<string, string> fullMetadata(metadata);
  // QC metadata (prefix qc_)
  fullMetadata["qc_version"] = Version::GetQcVersion().getString();
  fullMetadata["qc_detector_name"] = detectorName;
  fullMetadata["qc_task_name"] = taskName;
  fullMetadata["ObjectType"] = o2::utils::MemFileHelper::getClassName(typeInfo);

  // other attributes
  if (from == -1) {
    from = getCurrentTimestamp();
  }
  if (to == -1) {
    to = from + 1000l * 60 * 60 * 24 * 365 * 10; // ~10 years since the start of validity
  }

  ILOG(Debug, Support) << "Storing object " << path << " of type " << fullMetadata["ObjectType"] << ENDM;
  int result = ccdbApi.storeAsTFile_impl(obj, typeInfo, path, fullMetadata, from, to, mMaxObjectSize);

  if (result == -1 /* object bigger than maxObjectSize */) {
    static AliceO2::InfoLogger::InfoLogger::AutoMuteToken msgLimit(LogWarningSupport, 1, 600); // send it once every 10 minutes
    string msg = "object " + path + " is bigger than the maximum allowed size (" + to_string(mMaxObjectSize) + "B) - skipped";
    ILOG_INST.log(msgLimit, "%s", msg.c_str());
  }
}

// Monitor object
void CcdbDatabase::storeMO(std::shared_ptr<const o2::quality_control::core::MonitorObject> mo, long from, long to)
{
  if (mo->getName().length() == 0 || mo->getTaskName().length() == 0) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't be empty. Do not store. "));
  }

  if (mo->getName().find_first_of("\t\n ") != string::npos || mo->getTaskName().find_first_of("\t\n ") != string::npos) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't contain white spaces. Do not store."));
  }

  map<string, string> metadata;

  // user metadata
  map<string, string> userMetadata = mo->getMetadataMap();
  if (!userMetadata.empty()) {
    metadata.insert(userMetadata.begin(), userMetadata.end());
  }

  // extract object and metadata from MonitorObject
  TObject* obj = mo->getObject();
  metadata["RunNumber"] = std::to_string(mo->getActivity().mId);
  metadata["PeriodName"] = mo->getActivity().mPeriodName;
  metadata["PassName"] = mo->getActivity().mPassName;
  metadata["ObjectType"] = mo->getObject()->IsA()->GetName(); // ObjectType says TObject and not MonitorObject due to a quirk in the API. Once fixed, remove this.

  // QC metadata (prefix qc_)
  metadata["qc_version"] = Version::GetQcVersion().getString();
  metadata["qc_detector_name"] = mo->getDetectorName();
  metadata["qc_task_name"] = mo->getTaskName();
  metadata["qc_task_class"] = mo->getTaskClass();

  // path attributes
  string path = mo->getPath();
  if (from == -1) {
    from = getCurrentTimestamp();
  }
  if (to == -1) {
    to = from + 1000l * 60 * 60 * 24 * 365 * 10; // ~10 years since the start of validity
  }

  ILOG(Debug, Support) << "Storing MonitorObject " << path << ENDM;
  int result = ccdbApi.storeAsTFileAny<TObject>(obj, path, metadata, from, to, mMaxObjectSize);

  if (result == -1 /* object bigger than maxObjectSize */) {
    static AliceO2::InfoLogger::InfoLogger::AutoMuteToken msgLimit(LogWarningSupport, 1, 600); // send it once every 10 minutes
    string msg = "object " + path + " is bigger than the maximum allowed size (" + to_string(mMaxObjectSize) + "B) - skipped";
    ILOG_INST.log(msgLimit, "%s", msg.c_str());
  }
}

void CcdbDatabase::storeQO(std::shared_ptr<const o2::quality_control::core::QualityObject> qo, long from, long to)
{
  // metadata
  map<string, string> metadata;
  metadata["RunNumber"] = std::to_string(qo->getActivity().mId);
  metadata["PeriodName"] = qo->getActivity().mPeriodName;
  metadata["PassName"] = qo->getActivity().mPassName;
  metadata["ObjectType"] = qo->IsA()->GetName(); // ObjectType says TObject and not MonitorObject due to a quirk in the API. Once fixed, remove this.
  // QC metadata (prefix qc_)
  metadata["qc_version"] = Version::GetQcVersion().getString();
  metadata["qc_quality"] = std::to_string(qo->getQuality().getLevel());
  metadata["qc_detector_name"] = qo->getDetectorName();
  metadata["qc_check_name"] = qo->getCheckName();
  // user metadata
  map<string, string> userMetadata = qo->getMetadataMap();
  if (!userMetadata.empty()) {
    metadata.insert(userMetadata.begin(), userMetadata.end());
  }

  // other attributes
  string path = qo->getPath();
  if (from == -1) {
    from = getCurrentTimestamp();
  }
  if (to == -1) {
    to = from + 1000l * 60 * 60 * 24 * 365 * 10; // ~10 years since the start of validity
  }

  ILOG(Debug, Support) << "Storing quality object " << path << " (" << qo->getName() << ")" << ENDM;
  int result = ccdbApi.storeAsTFileAny<QualityObject>(qo.get(), path, metadata, from, to);

  if (result == -1 /* object bigger than maxObjectSize */) {
    static AliceO2::InfoLogger::InfoLogger::AutoMuteToken msgLimit(LogWarningSupport, 1, 600); // send it once every 10 minutes
    string msg = "object " + path + " is bigger than the maximum allowed size (" + to_string(mMaxObjectSize) + "B) - skipped";
    ILOG_INST.log(msgLimit, "%s", msg.c_str());
  }
}

void CcdbDatabase::storeTRFC(std::shared_ptr<const o2::quality_control::TimeRangeFlagCollection> trfc)
{
  // metadata
  map<string, string> metadata;
  metadata["RunNumber"] = std::to_string(trfc->getRunNumber());
  metadata["PeriodName"] = trfc->getPeriodName();
  metadata["PassName"] = trfc->getPassName();
  metadata["ObjectType"] = trfc->IsA()->GetName();
  // QC metadata (prefix qc_)
  metadata["qc_version"] = Version::GetQcVersion().getString();
  metadata["qc_trfc_name"] = trfc->getName();
  metadata["qc_detector_name"] = trfc->getDetector();

  // other attributes
  string path = RepoPathUtils::getTrfcPath(trfc.get());
  auto from = trfc->getStart();
  auto to = trfc->getEnd();
  if (from > to) {
    ILOG(Error, Support) << "TRFC '" + trfc->getName() + "' cannot be stored in CCDB, because it has invalid validity range (" + std::to_string(from) + ", " + std::to_string(to) + ")." << ENDM;
  }
  std::stringstream buffer;
  trfc->streamTo(buffer);
  ILOG(Debug, Support) << "Storing TimeRangeFlagCollection at " << path << " (" << trfc->getName() << ")" << ENDM;
  auto result = ccdbApi.storeAsBinaryFile(buffer.str().c_str(), buffer.str().size(), trfc->getName(), trfc->IsA()->GetName(), path, metadata, from, to);
  if (result != 0) {
    ILOG(Error, Support) << "TRFC '" + trfc->getName() + "' could not be stored in CCDB, error: " + std::to_string(result) << ENDM;
  }
}

TObject* CcdbDatabase::retrieveTObject(std::string path, std::map<std::string, std::string> const& metadata, long timestamp, std::map<std::string, std::string>* headers)
{
  // we try first to load a TFile
  auto* object = ccdbApi.retrieveFromTFileAny<TObject>(path, metadata, timestamp, headers);
  if (object == nullptr) {
    ILOG(Error, Support) << "We could NOT retrieve the object " << path << "." << ENDM;
    return nullptr;
  }
  ILOG(Debug, Support) << "Retrieved object " << path << " with timestamp " << timestamp << ENDM;
  return object;
}

void* CcdbDatabase::retrieveAny(const type_info& tinfo, const string& path, const map<std::string, std::string>& metadata, long timestamp, std::map<std::string, std::string>* headers, const string& createdNotAfter, const string& createdNotBefore)
{
  auto* object = ccdbApi.retrieveFromTFile(tinfo, path, metadata, timestamp, headers, "", createdNotAfter, createdNotBefore);
  if (object == nullptr) {
    ILOG(Error, Support) << "We could NOT retrieve the object " << path << "." << ENDM;
    return nullptr;
  }
  ILOG(Debug, Support) << "Retrieved object " << path << " with timestamp " << timestamp << ENDM;
  return object;
}

std::shared_ptr<o2::quality_control::core::MonitorObject> CcdbDatabase::retrieveMO(std::string objectPath, std::string objectName, long timestamp, const core::Activity& activity)
{
  string path = objectPath + "/" + objectName;
  map<string, string> headers;
  map<string, string> metadata = database_helpers::asDatabaseMetadata(activity);
  TObject* obj = retrieveTObject(path, metadata, timestamp, &headers);

  // no object found
  if (obj == nullptr) {
    if (headers.count("Error") > 0) {
      ILOG(Error, Support) << headers["Error"] << ENDM;
    }
    return nullptr;
  }

  // retrieve headers to determine the version of the QC framework
  Version objectVersion(headers["qc_version"]);
  ILOG(Debug, Devel) << "Version of object is " << objectVersion << ENDM;

  std::shared_ptr<MonitorObject> mo;
  if (objectVersion == Version("0.0.0") || objectVersion < Version("0.25")) {
    ILOG(Debug, Devel) << "Version of object " << objectPath << "/" << objectName << " is < 0.25" << ENDM;
    // The object is either in a TFile or is a blob but it was stored with storeAsTFile as a full MO
    mo.reset(dynamic_cast<MonitorObject*>(obj));
    if (mo == nullptr) {
      ILOG(Error, Devel) << "Could not cast the object " << objectPath << "/" << objectName << " to MonitorObject" << ENDM;
    }
  } else {
    // Version >= 0.25 -> the object is stored directly unencapsulated
    ILOG(Debug, Devel) << "Version of object " << objectPath << "/" << objectName << " is >= 0.25" << ENDM;
    int runNumber = stoi(headers["RunNumber"]);
    string provenance = path.substr(0, path.find('/')); // get the item before the first slash corresponding to the provenance
    mo = make_shared<MonitorObject>(obj, headers["qc_task_name"], headers["qc_task_class"], headers["qc_detector_name"], runNumber, headers["PeriodName"], headers["PassName"], provenance);
    // TODO should we remove the headers we know are general such as ETag and qc_task_name ?
    mo->addMetadata(headers);
  }
  mo->setIsOwner(true);
  return mo;
}

std::shared_ptr<o2::quality_control::core::QualityObject> CcdbDatabase::retrieveQO(std::string qoPath, long timestamp, const core::Activity& activity)
{
  map<string, string> headers;
  map<string, string> metadata = database_helpers::asDatabaseMetadata(activity);
  TObject* obj = retrieveTObject(qoPath, metadata, timestamp, &headers);
  std::shared_ptr<QualityObject> qo(dynamic_cast<QualityObject*>(obj));
  if (qo == nullptr) {
    ILOG(Error, Devel) << "Could not cast the object " << qoPath << " to QualityObject" << ENDM;
  } else {
    // TODO should we remove the headers we know are general such as ETag and qc_task_name ?
    qo->addMetadata(headers);
  }
  return qo;
}

std::shared_ptr<o2::quality_control::TimeRangeFlagCollection> CcdbDatabase::retrieveTRFC(const std::string& trfcName, const std::string& detector, int runNumber, const string& passName, const string& periodName, const std::string& provenance, long timestamp)
{
  map<string, string> headers;
  map<string, string> metadata;
  if (runNumber != 0) {
    metadata["RunNumber"] = std::to_string(runNumber);
  }
  if (!passName.empty()) {
    metadata["PassName"] = passName;
  }
  if (!periodName.empty()) {
    metadata["PeriodName"] = periodName;
  }
  const auto trfcPath = RepoPathUtils::getTrfcPath(detector, trfcName, provenance);
  const std::string localFileDir = "/tmp";
  const std::string localFileName = "trfc_" + trfcName + std::to_string(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count());
  const std::string localFilePath = localFileDir + std::filesystem::path::preferred_separator + localFileName;
  if (localFilePath.find("..") != std::string::npos || localFilePath.find('~') != std::string::npos) {
    ILOG(Error, Support) << "The path '" << localFilePath << "' looks hacky, will not download any files there." << ENDM;
    return nullptr;
  }

  auto resultMetadata = ccdbApi.retrieveHeaders(trfcPath, metadata, timestamp);
  if (resultMetadata.empty()) {
    ILOG(Error, Support) << "Could not extract headers of TRFC at '" << trfcPath << "' with the metadata: " << ENDM; // TODO
    ILOG(Error, Support) << " - RunNumber  : " << metadata["RunNumber"] << ENDM;
    ILOG(Error, Support) << " - PassName   : " << metadata["PassName"] << ENDM;
    ILOG(Error, Support) << " - PeriodName : " << metadata["PeriodName"] << ENDM;
    return nullptr;
  }

  auto success = ccdbApi.retrieveBlob(trfcPath, localFileDir, metadata, timestamp, false, localFileName);
  if (!success) {
    ILOG(Error, Support) << "Could not retrieve the TRFC at '" << trfcPath << "' with the metadata: " << ENDM; // TODO
    ILOG(Error, Support) << " - RunNumber  : " << metadata["RunNumber"] << ENDM;
    ILOG(Error, Support) << " - PassName   : " << metadata["PassName"] << ENDM;
    ILOG(Error, Support) << " - PeriodName : " << metadata["PeriodName"] << ENDM;
    return nullptr;
  }

  std::ifstream localFile(localFilePath);
  if (!localFile.is_open()) {
    ILOG(Error, Support) << "Could not open a file at '" << localFilePath << "'" << ENDM;
    std::filesystem::remove(localFilePath);
    return nullptr;
  }

  TimeRangeFlagCollection::RangeInterval validity{ std::stoull(resultMetadata["Valid-From"]), std::stoull(resultMetadata["Valid-Until"]) };
  auto trfc = std::make_shared<TimeRangeFlagCollection>(trfcName, detector, validity, runNumber, periodName, passName, provenance);
  trfc->streamFrom(localFile);
  localFile.close();
  std::filesystem::remove(localFilePath);

  return trfc;
}

std::string CcdbDatabase::retrieveJson(std::string path, long timestamp, const std::map<std::string, std::string>& metadata)
{
  map<string, string> headers;
  Document jsonDocument;

  // Get object
  auto* tobj = retrieveTObject(path, metadata, timestamp, &headers);
  if (tobj == nullptr) {
    return std::string();
  }

  // Convert object to JSON string
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
    ILOG(Error, Support) << "Unable to get the object to convert" << ENDM;
    return std::string();
  }
  TString json = TBufferJSON::ConvertToJSON(toConvert);
  delete toConvert;

  // Prepare JSON document and add metadata
  if (jsonDocument.Parse(json.Data()).HasParseError()) {
    ILOG(Error, Support) << "Unable to parse the JSON returned by TBufferJSON for object " << path << ENDM;
    return std::string();
  }
  rapidjson::Document::AllocatorType& allocator = jsonDocument.GetAllocator();
  rapidjson::Value object(rapidjson::Type::kObjectType);
  for (auto const& [key, value] : headers) {
    rapidjson::Value k(key.c_str(), allocator);
    rapidjson::Value v(value.c_str(), allocator);
    object.AddMember(k, v, allocator);
  }
  jsonDocument.AddMember("metadata", object, allocator);

  // Convert to string
  StringBuffer buffer;
  buffer.Clear();
  Writer<rapidjson::StringBuffer> writer(buffer);
  jsonDocument.Accept(writer);
  if (auto maxSize = std::string().max_size(); buffer.GetSize() > maxSize) {
    throw std::runtime_error("JSON buffer contains too large string: " + std::to_string(buffer.GetSize()) + ", while std::string::max_size() is " + maxSize);
  }
  return strdup(buffer.GetString());
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

std::vector<uint64_t> CcdbDatabase::getTimestampsForObject(std::string path)
{
  std::stringstream listingAsStringStream{ getListingAsString(path, "application/json") };
  //  std::cout << "listingAsString: " << listingAsStringStream.str() << std::endl;

  boost::property_tree::ptree listingAsTree;
  boost::property_tree::read_json(listingAsStringStream, listingAsTree);

  std::vector<uint64_t> timestamps;
  const auto& objects = listingAsTree.get_child("objects");
  timestamps.reserve(objects.size());

  // As for today, we receive objects in the order of the newest to the oldest.
  // We prefer the other order here.
  for (auto rit = objects.rbegin(); rit != objects.rend(); ++rit) {
    timestamps.emplace_back(rit->second.get<uint64_t>("Valid-From"));
  }

  // we make sure it is sorted. If it is already, it shouldn't cost much.
  std::sort(timestamps.begin(), timestamps.end());
  return timestamps;
}

std::vector<std::string> CcdbDatabase::getPublishedObjectNames(std::string taskName)
{
  std::vector<string> result;
  string listing = ccdbApi.list(taskName + "/.*", true, "Application/JSON");

  boost::property_tree::ptree pt;
  stringstream ss;
  ss << listing;
  boost::property_tree::read_json(ss, pt);

  BOOST_FOREACH (boost::property_tree::ptree::value_type& v, pt.get_child("objects")) {
    assert(v.first.empty()); // array elements have no names
    string data = v.second.get_child("path").data();
    string path = data.substr(taskName.size());
    result.push_back(path);
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
  ILOG(Info, Support) << "Truncating data for " << taskName << "/" << objectName << ENDM;

  ccdbApi.truncate(taskName + "/" + objectName);
}

void CcdbDatabase::storeStreamerInfosToFile(std::string filename)
{
  TH1F* h1 = new TH1F("asdf", "asdf", 100, 0, 99);
  shared_ptr<MonitorObject> mo1 = make_shared<MonitorObject>(h1, "fake", "class", "DET");
  TMessage message(kMESS_OBJECT);
  message.Reset();
  message.EnableSchemaEvolution(true);
  message.WriteObjectAny(mo1.get(), mo1->IsA());
  TList* infos = message.GetStreamerInfos();
  TFile f(filename.data(), "recreate");
  infos->Write();
  f.Close();
}

void CcdbDatabase::setMaxObjectSize(size_t maxObjectSize)
{
  CcdbDatabase::mMaxObjectSize = maxObjectSize;
}

} // namespace o2::quality_control::repository
