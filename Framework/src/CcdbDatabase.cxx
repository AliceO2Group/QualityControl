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
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/ObjectMetadataKeys.h"

// O2
#include <Common/Exceptions.h>
#include <CCDB/CcdbApi.h>
#include <CommonUtils/MemFileHelper.h>
// ROOT
#include <TBufferJSON.h>
#include <TH1F.h>
#include <TFile.h>
#include <TList.h>
#include <TROOT.h>
#include <TKey.h>
// std
#include <chrono>
#include <sstream>
#include <filesystem>
// boost
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/json_parser/error.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <utility>
// misc
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <InfoLogger/InfoLoggerMacros.hxx>

using namespace std::chrono;
using namespace AliceO2::Common;
using namespace o2::quality_control::core;
using namespace std;
using namespace rapidjson;

namespace o2::quality_control::repository
{
CcdbDatabase::CcdbDatabase() : ccdbApi(new o2::ccdb::CcdbApi())
{
}

CcdbDatabase::~CcdbDatabase() { disconnect(); }

void CcdbDatabase::connect(const string& host, const string& /*database*/, const string& /*username*/, const string& /*password*/)
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
  ccdbApi->init(mUrl);
  ccdbApi->setCurlRetriesParameters(5);
}

void CcdbDatabase::handleStorageError(const string& path, int result)
{
  if (result == -1 /* object bigger than maxObjectSize */) {
    static AliceO2::InfoLogger::InfoLogger::AutoMuteToken msgLimit(LogWarningSupport, 1, 600); // send it once every 10 minutes
    string msg = "object " + path + " is bigger than the maximum allowed size (" + to_string(mMaxObjectSize) + "B) - skipped";
    ILOG_INST.log(msgLimit, "%s", msg.c_str());
  }

  if (result == -2 /* curl initialization error */ || result > 0 /* curl error */) {
    mDatabaseFailure = true;
    mFailureTimer.reset(mFailureDelay * 1000000);
    string msg = "Unable to store object " + path + ". Next attempt to store objects in " + to_string(mFailureDelay) + " seconds.";
    ILOG(Warning, Ops) << msg << ENDM;
  }
}

bool CcdbDatabase::isDbInFailure()
{
  if (mDatabaseFailure) {
    if (mFailureTimer.isTimeout()) {
      mDatabaseFailure = false;
    } else {
      ILOG(Debug, Devel) << "Storage is disabled following a failure, this object won't be stored. New attempt in " << (int)mFailureTimer.getRemainingTime() << " seconds" << ENDM;
      return true;
    }
  }
  return false;
}

void CcdbDatabase::addFrameworkMetadata(map<string, string>& fullMetadata, string detectorName, string className)
{
  fullMetadata[metadata_keys::qcVersion] = Version::GetQcVersion().getString();
  fullMetadata[metadata_keys::qcDetectorCode] = std::move(detectorName);
  fullMetadata[metadata_keys::qcAdjustableEOV] = "1"; // QC-936 : this is to allow the modification of the end of validity.
  // ObjectType says TObject and not MonitorObject due to a quirk in the API. Once fixed, remove this.
  fullMetadata[metadata_keys::objectType] = std::move(className);
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

  if (isDbInFailure()) {
    return;
  }

  // metadata
  map<string, string> fullMetadata(metadata);
  addFrameworkMetadata(fullMetadata, detectorName, o2::utils::MemFileHelper::getClassName(typeInfo));
  fullMetadata[metadata_keys::qcTaskName] = taskName;

  // other attributes
  if (from == -1) {
    from = getCurrentTimestamp();
  }
  if (to == -1) {
    to = from + 1000l * 60 * 60 * 24 * 365 * 10; // ~10 years since the start of validity
  }

  ILOG(Debug, Support) << "Storing object " << path << " of type " << fullMetadata[metadata_keys::objectType] << ENDM;
  int result = ccdbApi->storeAsTFile_impl(obj, typeInfo, path, fullMetadata, from, to, mMaxObjectSize);

  handleStorageError(path, result);
}

// Monitor object
void CcdbDatabase::storeMO(std::shared_ptr<const o2::quality_control::core::MonitorObject> mo)
{
  if (mo->getName().length() == 0 || mo->getTaskName().length() == 0) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't be empty. Do not store. "));
  }

  if (mo->getName().find_first_of("\t\n ") != string::npos || mo->getTaskName().find_first_of("\t\n ") != string::npos) {
    BOOST_THROW_EXCEPTION(DatabaseException()
                          << errinfo_details("Object and task names can't contain white spaces. Do not store."));
  }

  if (isDbInFailure()) {
    return;
  }

  map<string, string> metadata = activity_helpers::asDatabaseMetadata(mo->getActivity());

  // user metadata
  map<string, string> userMetadata = mo->getMetadataMap();
  if (!userMetadata.empty()) {
    metadata.insert(userMetadata.begin(), userMetadata.end());
  }

  // extract object and metadata from MonitorObject
  TObject* obj = mo->getObject();

  // QC metadata (prefix qc_)
  addFrameworkMetadata(metadata, mo->getDetectorName(), mo->getObject()->IsA()->GetName());
  metadata[metadata_keys::qcTaskName] = mo->getTaskName();
  metadata[metadata_keys::qcTaskClass] = mo->getTaskClass();

  // path attributes
  string path = mo->getPath();
  auto validity = mo->getValidity();
  auto from = static_cast<long>(validity.getMin());
  auto to = static_cast<long>(validity.getMax());

  if (from == -1 || from == 0 || validity.getMin() == gInvalidValidityInterval.getMin() || validity.getMin() == gFullValidityInterval.getMin()) {
    from = getCurrentTimestamp();
  }
  if (to == -1 || to == 0 || validity.getMax() == gInvalidValidityInterval.getMax() || validity.getMax() == gFullValidityInterval.getMax()) {
    to = from + 1000l * 60 * 60 * 24 * 365 * 10; // ~10 years since the start of validity
  }
  if (from == to) {
    ILOG(Warning, Support) << "The validity start of '" << mo->GetName() << "' is equal to validity end (" << from << ", " << to << "). The validity end will be extended by 1ms to allow for storage." << ENDM;
    to += 1;
  }

  if (from > to) {
    ILOG(Error, Support) << "The validity start of '" << mo->GetName() << "' later than the end (" << from << ", " << to << "). The object will not be stored" << ENDM;
    return;
  }

  ILOG(Debug, Support) << "Storing MonitorObject " << path << ENDM;
  int result = ccdbApi->storeAsTFileAny<TObject>(obj, path, metadata, from, to, mMaxObjectSize);

  handleStorageError(path, result);
}

void CcdbDatabase::storeQO(std::shared_ptr<const o2::quality_control::core::QualityObject> qo)
{
  if (isDbInFailure()) {
    return;
  }

  // metadata
  map<string, string> metadata = activity_helpers::asDatabaseMetadata(qo->getActivity());
  // QC metadata (prefix qc_)
  addFrameworkMetadata(metadata, qo->getDetectorName(), qo->IsA()->GetName());
  metadata[metadata_keys::qcQuality] = std::to_string(qo->getQuality().getLevel());
  metadata[metadata_keys::qcCheckName] = qo->getCheckName();
  // user metadata
  map<string, string> userMetadata = qo->getMetadataMap();
  if (!userMetadata.empty()) {
    metadata.insert(userMetadata.begin(), userMetadata.end());
  }

  // other attributes
  string path = qo->getPath();
  auto validity = qo->getValidity();
  auto from = static_cast<long>(validity.getMin());
  auto to = static_cast<long>(validity.getMax());

  if (from == -1 || from == 0 || validity.getMin() == gInvalidValidityInterval.getMin() || validity.getMin() == gFullValidityInterval.getMin()) {
    from = getCurrentTimestamp();
  }
  if (to == -1 || to == 0 || validity.getMax() == gInvalidValidityInterval.getMax() || validity.getMax() == gFullValidityInterval.getMax()) {
    to = from + 1000l * 60 * 60 * 24 * 365 * 10; // ~10 years since the start of validity
  }

  if (from == to) {
    ILOG(Warning, Support) << "The validity start of '" << qo->GetName() << "' is equal to validity end (" << from << ", " << to << "). The validity end will be extended by 1ms to allow for storage." << ENDM;
    to += 1;
  }

  if (from > to) {
    ILOG(Error, Support) << "The validity start of '" << qo->GetName() << "' later than the end (" << from << ", " << to << "). The object will not be stored" << ENDM;
    return;
  }

  ILOG(Debug, Support) << "Storing quality object " << path << " (" << qo->getName() << ")" << ENDM;
  int result = ccdbApi->storeAsTFileAny<QualityObject>(qo.get(), path, metadata, from, to);

  handleStorageError(path, result);
}

TObject* CcdbDatabase::retrieveTObject(std::string path, std::map<std::string, std::string> const& metadata, long timestamp, std::map<std::string, std::string>* headers)
{
  if (timestamp == Timestamp::Latest) {
    auto latestValidity = getLatestObjectValidity(path, metadata);
    if (latestValidity.isInvalid()) {
      return nullptr;
    }
    timestamp = latestValidity.getMin();
  }
  // we try first to load a TFile
  auto* object = ccdbApi->retrieveFromTFileAny<TObject>(path, metadata, timestamp, headers);
  if (object == nullptr) {
    ILOG(Warning, Support) << "We could NOT retrieve the object " << path << " with timestamp " << timestamp << "." << ENDM;
    ILOG(Debug, Support) << "and with metadata:" << ENDM;
    for (auto [metaKey, metaVal] : metadata) {
      ILOG(Debug, Support) << metaKey << ", " << metaVal << ENDM;
    }
    return nullptr;
  }
  ILOG(Debug, Support) << "Retrieved object " << path << " with timestamp " << timestamp << ENDM;
  return object;
}

void* CcdbDatabase::retrieveAny(const type_info& tinfo, const string& path, const map<std::string, std::string>& metadata, long timestamp, std::map<std::string, std::string>* headers, const string& createdNotAfter, const string& createdNotBefore)
{
  if (timestamp == Timestamp::Latest) {
    auto latestValidity = getLatestObjectValidity(path, metadata);
    if (latestValidity.isInvalid()) {
      return nullptr;
    }
    timestamp = latestValidity.getMin();
  }
  auto* object = ccdbApi->retrieveFromTFile(tinfo, path, metadata, timestamp, headers, "", createdNotAfter, createdNotBefore);
  if (object == nullptr) {
    ILOG(Warning, Support) << "We could NOT retrieve the object " << path << " with timestamp " << timestamp << "." << ENDM;
    return nullptr;
  }
  ILOG(Debug, Support) << "Retrieved object " << path << " with timestamp " << timestamp << ENDM;
  return object;
}

std::shared_ptr<o2::quality_control::core::MonitorObject> CcdbDatabase::retrieveMO(std::string objectPath, std::string objectName,
                                                                                   long timestamp, const core::Activity& activity,
                                                                                   const std::map<std::string, std::string>& metadataToRetrieve)
{
  string fullPath = activity.mProvenance + "/" + objectPath + "/" + objectName;
  map<string, string> headers;
  map<string, string> metadata = activity_helpers::asDatabaseMetadata(activity, false);
  metadata.insert(metadataToRetrieve.begin(), metadataToRetrieve.end());
  TObject* obj = retrieveTObject(fullPath, metadata, timestamp, &headers);

  // no object found
  if (obj == nullptr) {
    if (headers.count("Error") > 0) {
      ILOG(Error, Support) << headers["Error"] << ENDM;
    }
    return nullptr;
  }

  // retrieve headers to determine the version of the QC framework
  Version objectVersion(headers[metadata_keys::qcVersion]);
  ILOG(Debug, Devel) << "Version of object is " << objectVersion << ENDM;

  std::shared_ptr<MonitorObject> mo;
  if (objectVersion == Version("0.0.0") || objectVersion < Version("0.25")) {
    ILOG(Debug, Devel) << "Version of object " << fullPath << " is < 0.25" << ENDM;
    // The object is either in a TFile or is a blob but it was stored with storeAsTFile as a full MO
    mo.reset(dynamic_cast<MonitorObject*>(obj));
    if (mo == nullptr) {
      ILOG(Error, Devel) << "Could not cast the object " << fullPath << " to MonitorObject (objectVersion: " << objectVersion << ")" << ENDM;
      return nullptr;
    }
  } else {
    // Version >= 0.25 -> the object is stored directly unencapsulated
    ILOG(Debug, Devel) << "Version of object " << fullPath << " is >= 0.25" << ENDM;
    mo = make_shared<MonitorObject>(obj, headers[metadata_keys::qcTaskName], headers[metadata_keys::qcTaskClass], headers[metadata_keys::qcDetectorCode]);
    // TODO should we remove the headers we know are general such as ETag and qc_task_name ?
    mo->addMetadata(headers);
    // we could just copy the argument here, but this would not cover cases where the activity in headers has more non-default fields
    mo->setActivity(activity_helpers::asActivity(headers, activity.mProvenance));
  }
  mo->setIsOwner(true);
  return mo;
}

std::shared_ptr<o2::quality_control::core::QualityObject> CcdbDatabase::retrieveQO(std::string qoPath, long timestamp,
                                                                                   const core::Activity& activity,
                                                                                   const std::map<std::string, std::string>& metadataToRetrieve)
{
  map<string, string> headers;
  map<string, string> metadata = activity_helpers::asDatabaseMetadata(activity, false);
  metadata.insert(metadataToRetrieve.begin(), metadataToRetrieve.end());
  auto fullPath = activity.mProvenance + "/" + qoPath;
  TObject* obj = retrieveTObject(fullPath, metadata, timestamp, &headers);
  if (obj == nullptr) {
    return nullptr;
  }
  std::shared_ptr<QualityObject> qo(dynamic_cast<QualityObject*>(obj));
  if (qo == nullptr) {
    ILOG(Error, Devel) << "Could not cast the object " << fullPath << " to QualityObject" << ENDM;
  } else {
    // TODO should we remove the headers we know are general such as ETag and qc_task_name ?
    qo->addMetadata(headers);
    // we could just copy the argument here, but this would not cover cases where the activity in headers has more non-default fields
    qo->setActivity(activity_helpers::asActivity(headers, activity.mProvenance));
  }
  return qo;
}

std::string CcdbDatabase::retrieveJson(std::string path, long timestamp, const std::map<std::string, std::string>& metadata)
{
  map<string, string> headers;
  Document jsonDocument;

  // Get object
  auto* tobj = retrieveTObject(path, metadata, timestamp, &headers);
  if (tobj == nullptr) {
    return {};
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
    return {};
  }
  TString json = TBufferJSON::ConvertToJSON(toConvert);
  delete toConvert;

  // Prepare JSON document and add metadata
  if (jsonDocument.Parse(json.Data()).HasParseError()) {
    ILOG(Error, Support) << "Unable to parse the JSON returned by TBufferJSON for object " << path << ENDM;
    return {};
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

std::string CcdbDatabase::getListingAsString(const std::string& subpath, const std::string& accept, bool latestOnly)
{
  return ccdbApi->list(subpath, latestOnly, accept);
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

std::vector<std::string> CcdbDatabase::getListing(const std::string& subpath)
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

boost::property_tree::ptree CcdbDatabase::getListingAsPtree(const std::string& path, const std::map<std::string, std::string>& metadata, bool latestOnly)
{
  // CCDB accepts metadata filters as slash-separated key=value pairs at the end of the object path
  std::stringstream pathWithMetadata;
  pathWithMetadata << path;
  for (const auto& [key, value] : metadata) {
    pathWithMetadata << '/' << key << '=' << value;
  }

  std::stringstream listingAsStringStream{ getListingAsString(pathWithMetadata.str(), "application/json", latestOnly) };

  boost::property_tree::ptree listingAsTree;
  try {
    boost::property_tree::read_json(listingAsStringStream, listingAsTree);
  } catch (const boost::property_tree::json_parser::json_parser_error&) {
    ILOG(Error, Support) << "Failed to parse json in CcdbDatabase::getListingAsPtree from data: " << listingAsStringStream.str() << ENDM;
  }

  return listingAsTree;
}

core::ValidityInterval CcdbDatabase::getLatestObjectValidity(const std::string& path, const std::map<std::string, std::string>& metadata)
{
  auto listing = getListingAsPtree(path, metadata, true);
  if (listing.count("objects") == 0) {
    ILOG(Warning, Support) << "Could not get a valid listing from db '" << mUrl << "' for latestObjectMetadata '" << path << "'" << ENDM;
    return gInvalidValidityInterval;
  }
  const auto& objects = listing.get_child("objects");
  if (objects.empty()) {
    return gInvalidValidityInterval;
  } else if (objects.size() > 1) {
    ILOG(Warning, Support) << "Expected just one metadata entry for object '" << path << "'. Trying to continue by using the first." << ENDM;
  }
  const auto& latestObjectMetadata = objects.front().second;

  return { latestObjectMetadata.get<uint64_t>(metadata_keys::validFrom), latestObjectMetadata.get<uint64_t>(metadata_keys::validUntil) };
}

std::vector<uint64_t> CcdbDatabase::getTimestampsForObject(const std::string& path)
{
  const auto& objects = getListingAsPtree(path).get_child("objects");
  std::vector<uint64_t> timestamps;
  timestamps.reserve(objects.size());

  // As for today, we receive objects in the order of the newest to the oldest.
  // We prefer the other order here.
  for (auto rit = objects.rbegin(); rit != objects.rend(); ++rit) {
    timestamps.emplace_back(rit->second.get<uint64_t>(metadata_keys::validFrom));
  }

  // we make sure it is sorted. If it is already, it shouldn't cost much.
  std::sort(timestamps.begin(), timestamps.end());
  return timestamps;
}

std::vector<std::string> CcdbDatabase::getPublishedObjectNames(std::string taskName)
{
  std::vector<string> result;
  string listing = ccdbApi->list(taskName + "/.*", true, "Application/JSON");

  boost::property_tree::ptree pt;
  stringstream ss;
  ss << listing;

  try {
    boost::property_tree::read_json(ss, pt);
  } catch (const boost::property_tree::json_parser::json_parser_error&) {
    ILOG(Error, Support) << "Failed to parse json in CcdbDatabase::getTimestampsForObject from data: " << ss.str() << ENDM;
  }

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

void CcdbDatabase::truncate(std::string path, std::string objectName)
{
  ILOG(Info, Support) << "Truncating data for " << path << "/" << objectName << ENDM;

  ccdbApi->truncate(path + "/" + objectName);
}

void CcdbDatabase::setMaxObjectSize(size_t maxObjectSize)
{
  CcdbDatabase::mMaxObjectSize = maxObjectSize;
}

} // namespace o2::quality_control::repository
