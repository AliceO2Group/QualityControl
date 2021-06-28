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
/// \file   CalDetPublisher.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "TPC/CalDetPublisher.h"
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/Utility.h"

//root includes
#include "TCanvas.h"

#include <fmt/format.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

const std::unordered_map<std::string, outputType> OutputMap{
  { "Pedestal", outputType::Pedestal },
  { "Noise", outputType::Noise }
};

void CalDetPublisher::configure(std::string name, const boost::property_tree::ptree& config)
{
  for (const auto& output : config.get_child("qc.postprocessing." + name + ".outputList")) {
    try {
      OutputMap.at(output.second.data());
      mOutputList.emplace_back(output.second.data());
    } catch (std::out_of_range&) {
      throw std::invalid_argument(std::string("Invalid output CalDet object specified in config: ") + output.second.data());
    }
  }

  for (const auto& data : config.get_child("qc.postprocessing." + name + ".metaData")) {
    mMetaKeys.emplace_back(std::vector<std::string>());
    mMetaValues.emplace_back(std::vector<std::string>());
    if (const auto& keys = data.second.get_child_optional("keys"); keys.has_value()) {
      for (const auto& key : keys.value()) {
        mMetaKeys.back().emplace_back(key.second.data());
      }
    } else {
      mMetaKeys.back().emplace_back(std::string());
    }
    if (const auto& values = data.second.get_child_optional("values"); values.has_value()) {
      for (const auto& value : values.value()) {
        mMetaValues.back().emplace_back(value.second.data());
      }
    } else {
      mMetaValues.back().emplace_back(std::string());
    }
  }

  auto iter = 0;
  for (auto& keyVec : mMetaKeys) {
    if (keyVec.size() != mMetaValues.at(iter).size()) {
      throw std::runtime_error("Number of keys and values for metaData are not matching");
    }
    if (keyVec.size() == 0) {
      throw std::runtime_error("Empty key or value lists are not allowed! If you do not want meta data, remove 'keys' and 'values'");
    }
    iter++;
  }
}

void CalDetPublisher::initialize(Trigger, framework::ServiceRegistry&)
{
  std::map<std::string, std::string> metaData{};

  auto keyIter = 0;
  if (mMetaKeys.size() == 1) {
    for (auto& key : mMetaKeys.at(0)) {
      metaData.insert(std::pair<std::string, std::string>(key, mMetaValues.at(0).at(keyIter)));
      keyIter++;
    }
  }

  auto CalDetIter = 0;
  for (auto& type : mOutputList) {
    if (mMetaKeys.size() > 1) {
      metaData.clear();
      keyIter = 0;
      for (auto& key : mMetaKeys.at(CalDetIter)) {
        metaData.insert(std::pair<std::string, std::string>(key, mMetaValues.at(CalDetIter).at(keyIter)));
        keyIter++;
      }
    }
    mCalDetCanvasVec.emplace_back(std::vector<std::unique_ptr<TCanvas>>());
    addAndPublish(getObjectsManager(), mCalDetCanvasVec.back(), { fmt::format("c_Sides_{}", type).data(), fmt::format("c_ROCs_{}_1D", type).data(), fmt::format("c_ROCs_{}_2D", type).data() }, metaData);
    CalDetIter++;
  }
}

void CalDetPublisher::update(Trigger t, framework::ServiceRegistry&)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  auto calDetIter = 0;
  for (auto& type : mOutputList) {
    auto& calDet = o2::tpc::CDBInterface::instance().getCalPad(fmt::format("TPC/Calib/{}", type).data());
    auto vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, 300, 0, 0, true, &vecPtr);
    calDetIter++;
  }
}

void CalDetPublisher::finalize(Trigger, framework::ServiceRegistry&)
{
  for (auto& calDetCanvasVec : mCalDetCanvasVec) {
    for (auto& canvas : calDetCanvasVec) {
      getObjectsManager()->stopPublishing(canvas.get());
    }
  }
}

} // namespace o2::quality_control_modules::tpc
