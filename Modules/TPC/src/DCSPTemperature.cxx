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
/// \file   DCSPTemperature.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "DataFormatsTPC/DCS.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/DCSPTemperature.h"

// root includes
#include "TCanvas.h"
#include "TGraph.h"
#include "TStyle.h"

#include <fmt/format.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void DCSPTemperature::configure(std::string name, const boost::property_tree::ptree& config)
{
  std::vector<std::string> keyVec{};
  std::vector<std::string> valueVec{};
  for (const auto& key : config.get_child("qc.postprocessing." + name + ".lookupMetaData.keys")) {
    keyVec.emplace_back(key.second.data());
  }
  for (const auto& value : config.get_child("qc.postprocessing." + name + ".lookupMetaData.values")) {
    valueVec.emplace_back(value.second.data());
  }

  int vecIter = 0;
  if ((keyVec.size() > 0) && (keyVec.size() == valueVec.size())) {
    for (const auto& key : keyVec) {
      mLookupMap.insert(std::pair<std::string, std::string>(key, valueVec.at(vecIter)));
      vecIter++;
    }
  }

  if (keyVec.size() != valueVec.size()) {
    throw std::runtime_error("Number of keys and values for lookupMetaData are not matching");
  }

  keyVec.clear();
  valueVec.clear();

  for (const auto& key : config.get_child("qc.postprocessing." + name + ".storeMetaData.keys")) {
    keyVec.emplace_back(key.second.data());
  }
  for (const auto& value : config.get_child("qc.postprocessing." + name + ".storeMetaData.values")) {
    valueVec.emplace_back(value.second.data());
  }

  vecIter = 0;
  if ((keyVec.size() > 0) && (keyVec.size() == valueVec.size())) {
    for (const auto& key : keyVec) {
      mStoreMap.insert(std::pair<std::string, std::string>(key, valueVec.at(vecIter)));
      vecIter++;
    }
  }

  mTimestamp = std::stol(config.get<std::string>("qc.postprocessing." + name + ".timestamp"));
  mNFiles = std::stoi(config.get<std::string>("qc.postprocessing." + name + ".nFiles"));
  mHost = config.get<std::string>("qc.config.conditionDB.url");
}

void DCSPTemperature::initialize(Trigger, framework::ServiceRegistry&)
{
  mCdbApi.init(mHost);
  mDCSPTemp.initializeCanvases();

  for (auto& canv : mDCSPTemp.getCanvases()) {
    getObjectsManager()->startPublishing(canv);
    for (const auto& [key, value] : mStoreMap) {
      getObjectsManager()->addMetadata(canv->GetName(), key, value);
    }
  }
}

void DCSPTemperature::update(Trigger, framework::ServiceRegistry&)
{
  std::vector<long> usedTimestamps = getDataTimestamps("TPC/Calib/Temperature", mNFiles, mTimestamp);
  for (auto& timestamp : usedTimestamps) {
    mData.emplace_back(mCdbApi.retrieveFromTFileAny<o2::tpc::dcs::Temperature>("TPC/Calib/Temperature", mLookupMap, timestamp));
  }

  mDCSPTemp.processData(mData);
}

void DCSPTemperature::finalize(Trigger, framework::ServiceRegistry&)
{
  for (auto& canv : mDCSPTemp.getCanvases()) {
    getObjectsManager()->stopPublishing(canv);
  }
}

std::vector<std::string> DCSPTemperature::splitString(const std::string inString, const char* delimiter)
{
  std::vector<std::string> outVec;
  std::string placeholder;
  std::istringstream stream(inString);
  std::string token;
  while (std::getline(stream, token, *delimiter)) {
    if (token == "") {
      if (!placeholder.empty()) {
        outVec.emplace_back(placeholder);
        placeholder.clear();
      }
    } else {
      placeholder.append(token + "\n");
    }
  }
  return std::move(outVec);
}

long DCSPTemperature::getTimestamp(const std::string metaInfo)
{
  std::string result_str;
  long result;
  std::string token = "Validity: ";
  int start = metaInfo.find(token) + token.size();
  int end = metaInfo.find(" -", start);
  result_str = metaInfo.substr(start, end - start);
  std::string::size_type sz;
  result = std::stol(result_str, &sz);
  return result;
}

std::vector<long> DCSPTemperature::getDataTimestamps(const std::string_view path, const unsigned int nFiles, const long limit)
{
  std::vector<long> outVec;
  std::vector<std::string> fileList = splitString(mCdbApi.list(path.data()), "\n");

  if (limit == -1) {
    for (unsigned int i = 0; i < nFiles; i++) {
      outVec.emplace_back(getTimestamp(fileList.at(i)));
    }
  } else {
    std::vector<std::string> tmpFiles;
    for (auto& file : fileList) {
      if (getTimestamp(file) <= limit) {
        tmpFiles.emplace_back(file);
      }
    }
    for (unsigned int i = 0; i < nFiles; i++) {
      outVec.emplace_back(getTimestamp(tmpFiles.at(i)));
    }
  }
  std::sort(outVec.begin(), outVec.end());
  return std::move(outVec);
}

} // namespace o2::quality_control_modules::tpc
