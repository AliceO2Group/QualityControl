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
#include "TPC/Utility.h"

// root includes
#include "TCanvas.h"
#include "TGraph.h"
#include "TStyle.h"

#include <fmt/format.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void DCSPTemperature::configure(const boost::property_tree::ptree& config)
{
  auto& id = getID();
  std::vector<std::string> keyVec{};
  std::vector<std::string> valueVec{};
  for (const auto& key : config.get_child("qc.postprocessing." + id + ".lookupMetaData.keys")) {
    keyVec.emplace_back(key.second.data());
  }
  for (const auto& value : config.get_child("qc.postprocessing." + id + ".lookupMetaData.values")) {
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
    ILOG(Error, Support) << "Number of keys and values for lookupMetaData are not matching" << ENDM;
  }

  keyVec.clear();
  valueVec.clear();

  for (const auto& key : config.get_child("qc.postprocessing." + id + ".storeMetaData.keys")) {
    keyVec.emplace_back(key.second.data());
  }
  for (const auto& value : config.get_child("qc.postprocessing." + id + ".storeMetaData.values")) {
    valueVec.emplace_back(value.second.data());
  }

  vecIter = 0;
  if ((keyVec.size() > 0) && (keyVec.size() == valueVec.size())) {
    for (const auto& key : keyVec) {
      mStoreMap.insert(std::pair<std::string, std::string>(key, valueVec.at(vecIter)));
      vecIter++;
    }
  }

  mTimestamp = std::stol(config.get<std::string>("qc.postprocessing." + id + ".timestamp"));
  mNFiles = std::stoi(config.get<std::string>("qc.postprocessing." + id + ".nFiles"));
  mHost = config.get<std::string>("qc.postprocessing." + id + ".dataSourceURL");
}

void DCSPTemperature::initialize(Trigger, framework::ServiceRegistryRef)
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

void DCSPTemperature::update(Trigger, framework::ServiceRegistryRef)
{
  std::vector<long> usedTimestamps = getDataTimestamps(mCdbApi, "TPC/Calib/Temperature", mNFiles, mTimestamp);
  for (auto& timestamp : usedTimestamps) {
    mData.emplace_back(mCdbApi.retrieveFromTFileAny<o2::tpc::dcs::Temperature>("TPC/Calib/Temperature", mLookupMap, timestamp));
  }

  mDCSPTemp.processData(mData);
}

void DCSPTemperature::finalize(Trigger, framework::ServiceRegistryRef)
{
  for (auto& canv : mDCSPTemp.getCanvases()) {
    getObjectsManager()->stopPublishing(canv);
  }
}

} // namespace o2::quality_control_modules::tpc
