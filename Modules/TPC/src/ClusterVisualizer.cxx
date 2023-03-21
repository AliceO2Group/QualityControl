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
/// \file   ClusterVisualizer.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"
#include "TPCQC/Helpers.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/ClusterVisualizer.h"
#include "TPC/Utility.h"
#include "TPC/ClustersData.h"

// root includes
#include "TCanvas.h"
#include "TPaveText.h"

#include <fmt/format.h>
#include <algorithm>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void ClusterVisualizer::configure(const boost::property_tree::ptree& config)
{
  auto& id = getID();

  for (const auto& timestamp : config.get_child("qc.postprocessing." + id + ".timestamps")) {
    mTimestamps.emplace_back(std::stol(timestamp.second.data()));
  }

  std::vector<std::string> keyVec{};
  std::vector<std::string> valueVec{};
  for (const auto& data : config.get_child("qc.postprocessing." + id + ".lookupMetaData")) {
    mLookupMaps.emplace_back(std::map<std::string, std::string>());
    if (const auto& keys = data.second.get_child_optional("keys"); keys.has_value()) {
      for (const auto& key : keys.value()) {
        keyVec.emplace_back(key.second.data());
      }
    }
    if (const auto& values = data.second.get_child_optional("values"); values.has_value()) {
      for (const auto& value : values.value()) {
        valueVec.emplace_back(value.second.data());
      }
    }
    auto vecIter = 0;
    if ((keyVec.size() > 0) && (keyVec.size() == valueVec.size())) {
      for (const auto& key : keyVec) {
        mLookupMaps.back().insert(std::pair<std::string, std::string>(key, valueVec.at(vecIter)));
        vecIter++;
      }
    }
    if (keyVec.size() != valueVec.size()) {
      ILOG(Error, Support) << "Number of keys and values for lookupMetaData are not matching" << ENDM;
    }
    keyVec.clear();
    valueVec.clear();
  }

  for (const auto& data : config.get_child("qc.postprocessing." + id + ".storeMetaData")) {
    mStoreMaps.emplace_back(std::map<std::string, std::string>());
    if (const auto& keys = data.second.get_child_optional("keys"); keys.has_value()) {
      for (const auto& key : keys.value()) {
        keyVec.emplace_back(key.second.data());
      }
    }
    if (const auto& values = data.second.get_child_optional("values"); values.has_value()) {
      for (const auto& value : values.value()) {
        valueVec.emplace_back(value.second.data());
      }
    }
    auto vecIter = 0;
    if ((keyVec.size() > 0) && (keyVec.size() == valueVec.size())) {
      for (const auto& key : keyVec) {
        mStoreMaps.back().insert(std::pair<std::string, std::string>(key, valueVec.at(vecIter)));
        vecIter++;
      }
    }
    if (keyVec.size() != valueVec.size()) {
      ILOG(Error, Support) << "Number of keys and values for storeMetaData are not matching" << ENDM;
    }
    keyVec.clear();
    valueVec.clear();
  }

  for (const auto& entry : config.get_child("qc.postprocessing." + id + ".histogramRanges")) {
    for (const auto& type : entry.second) {
      for (const auto& value : type.second) {
        mRanges[type.first].emplace_back(std::stof(value.second.data()));
      }
    }
  }

  mPath = config.get<std::string>("qc.postprocessing." + id + ".path");
  mHost = config.get<std::string>("qc.postprocessing." + id + ".dataSourceURL");

  const auto type = config.get<std::string>("qc.postprocessing." + id + ".dataType");
  if (type == "clusters") {
    mIsClusters = true;
    mObservables = {
      "N_Clusters",
      "Q_Max",
      "Q_Tot",
      "Sigma_Pad",
      "Sigma_Time",
      "Time_Bin"
    };
  } else if (type == "raw") {
    mIsClusters = false;
    mObservables = {
      "N_RawDigits",
      "Q_Max",
      "Time_Bin"
    };
  } else {
    ILOG(Error, Support) << "No valid data type given. 'dataType' has to be either 'clusters' or 'raw'." << ENDM;
  }
}

void ClusterVisualizer::initialize(Trigger, framework::ServiceRegistryRef)
{
  mCdbApi.init(mHost);

  auto calDetIter = 0;
  for (const auto& type : mObservables) {
    mCalDetCanvasVec.emplace_back(std::vector<std::unique_ptr<TCanvas>>());
    addAndPublish(getObjectsManager(),
                  mCalDetCanvasVec.back(),
                  { fmt::format("c_Sides_{}", type).data(),
                    fmt::format("c_ROCs_{}_1D", type).data(),
                    fmt::format("c_ROCs_{}_2D", type).data() },
                  mStoreMaps.size() > 1 ? mStoreMaps.at(calDetIter) : mStoreMaps.at(0));
    calDetIter++;
  }
}

void ClusterVisualizer::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  auto calDetIter = 0;

  auto clusterData = mCdbApi.retrieveFromTFileAny<ClustersData>(mPath,
                                                                mLookupMaps.size() > 1 ? mLookupMaps.at(calDetIter) : mLookupMaps.at(0),
                                                                mTimestamps.size() > 0 ? mTimestamps.at(calDetIter) : -1);

  auto& clusters = clusterData->getClusters();

  auto& calDet = clusters.getNClusters();
  auto vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
  o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
  calDetIter++;

  calDet = clusters.getQMax();
  vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
  o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
  calDetIter++;

  if (mIsClusters) {
    calDet = clusters.getQTot();
    vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;

    calDet = clusters.getSigmaPad();
    vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;

    calDet = clusters.getSigmaTime();
    vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;
  }

  calDet = clusters.getTimeBin();
  vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
  o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
  calDetIter++;
}

void ClusterVisualizer::finalize(Trigger t, framework::ServiceRegistryRef)
{
  for (const auto& calDetCanvasVec : mCalDetCanvasVec) {
    for (const auto& canvas : calDetCanvasVec) {
      getObjectsManager()->stopPublishing(canvas.get());
    }
  }
}

} // namespace o2::quality_control_modules::tpc
