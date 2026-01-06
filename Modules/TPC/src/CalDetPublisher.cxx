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
#if __has_include("TPCBase/Painter.h")
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"
#else
#include "TPCBaseRecSim/Painter.h"
#include "TPCBaseRecSim/CDBInterface.h"
#endif
#include "TPCQC/Helpers.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/CalDetPublisher.h"
#include "TPC/Utility.h"

// root includes
#include "TCanvas.h"
#include "TPaveText.h"

#include <fmt/format.h>
#include <boost/property_tree/ptree.hpp>
#include <algorithm>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void CalDetPublisher::configure(const boost::property_tree::ptree& config)
{
  o2::tpc::CDBInterface::instance().setURL(config.get<std::string>("qc.config.conditionDB.url"));

  auto& id = getID();
  for (const auto& output : config.get_child("qc.postprocessing." + id + ".outputCalPadMaps")) {
    mOutputListMap.emplace_back(output.second.data());
  }

  for (const auto& output : config.get_child("qc.postprocessing." + id + ".outputCalPads")) {
    mOutputList.emplace_back(output.second.data());
  }

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

  if ((mTimestamps.size() != mOutputList.size()) && (mTimestamps.size() > 0)) {
    ILOG(Error, Support) << "You need to set a timestamp for every CalPad object or none at all" << ENDM;
  }

  if (std::find(mOutputListMap.begin(), mOutputListMap.end(), "PedestalNoise") != mOutputListMap.end()) {
    mCheckZSPrereq = true;
  }

  for (const auto& entry : config.get_child("qc.postprocessing." + id + ".histogramRanges")) {
    for (const auto& type : entry.second) {
      for (const auto& value : type.second) {
        mRanges[type.first].emplace_back(std::stof(value.second.data()));
      }
    }
  }

  std::string checkZSCalibration = config.get<std::string>("qc.postprocessing." + id + ".checkZSCalibration.check");
  if (checkZSCalibration == "true" && mCheckZSPrereq == true) {
    mCheckZSCalib = true;
    mInitRefCalibTimestamp = std::stol(config.get<std::string>("qc.postprocessing." + id + ".checkZSCalibration.initRefCalibTimestamp"));
  } else if (checkZSCalibration == "false") {
    mCheckZSCalib = false;
  } else if (checkZSCalibration == "true" && mCheckZSPrereq == false) {
    ILOG(Error, Support) << "'PedestalNoise' needs to be in the 'outputCalPadMaps' to make the Zero Suppression calibration check" << ENDM;
  } else {
    ILOG(Error, Support) << "No valid value for 'checkZSCalibration.check' set. Has to be 'true' or 'false'" << ENDM;
  }

  o2::tpc::CDBInterface::instance().setURL(config.get<std::string>("qc.postprocessing." + id + ".dataSourceURL"));
}

void CalDetPublisher::initialize(Trigger, framework::ServiceRegistryRef)
{
  auto calDetIter = 0;
  for (const auto& type : mOutputListMap) {
    auto& calMap = o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<std::unordered_map<std::string, o2::tpc::CalDet<float>>>(fmt::format("TPC/Calib/{}", type).data(),
                                                                                                                                       -1,
                                                                                                                                       std::map<std::string, std::string>());
    for (const auto& item : calMap) {
      mCalDetCanvasVec.emplace_back(std::vector<std::unique_ptr<TCanvas>>());
      addAndPublish(getObjectsManager(),
                    mCalDetCanvasVec.back(),
                    { fmt::format("c_Sides_{}", item.second.getName()).data(),
                      fmt::format("c_ROCs_{}_1D", item.second.getName()).data(),
                      fmt::format("c_ROCs_{}_2D", item.second.getName()).data() },
                    mStoreMaps.size() > 1 ? mStoreMaps.at(calDetIter) : mStoreMaps.at(0));
    }
    calDetIter++;
  }

  for (const auto& type : mOutputList) {
    mCalDetCanvasVec.emplace_back(std::vector<std::unique_ptr<TCanvas>>());
    addAndPublish(getObjectsManager(),
                  mCalDetCanvasVec.back(),
                  { fmt::format("c_Sides_{}", type).data(),
                    fmt::format("c_ROCs_{}_1D", type).data(),
                    fmt::format("c_ROCs_{}_2D", type).data() },
                  mStoreMaps.size() > 1 ? mStoreMaps.at(calDetIter) : mStoreMaps.at(0));
    calDetIter++;
  }

  if (mCheckZSCalib) {
    auto& calMap = o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<std::unordered_map<std::string, o2::tpc::CalDet<float>>>("TPC/Calib/PedestalNoise",
                                                                                                                                       mInitRefCalibTimestamp,
                                                                                                                                       std::map<std::string, std::string>());
    mRefPedestal = std::make_unique<o2::tpc::CalDet<float>>(calMap["Pedestals"]);
    mRefNoise = std::make_unique<o2::tpc::CalDet<float>>(calMap["Noise"]);

    mNewZSCalibMsg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    mNewZSCalibMsg->AddText("Upload new calib data for ZS!");
    mNewZSCalibMsg->SetFillColor(kRed);
  }
}

void CalDetPublisher::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  auto calDetIter = 0;
  auto calVecIter = 0;
  for (const auto& type : mOutputListMap) {
    auto& calMap = o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<std::unordered_map<std::string, o2::tpc::CalDet<float>>>(
      fmt::format("TPC/Calib/{}", type).data(),
      mTimestamps.size() > 0 ? mTimestamps.at(calVecIter) : -1,
      mLookupMaps.size() > 1 ? mLookupMaps.at(calVecIter) : mLookupMaps.at(0));
    for (const auto& item : calMap) {
      auto vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
      o2::tpc::painter::makeSummaryCanvases(item.second, int(mRanges[item.second.getName()].at(0)), mRanges[item.second.getName()].at(1), mRanges[item.second.getName()].at(2), false, &vecPtr);
      calDetIter++;
    }
    calVecIter++;

    if (mCheckZSCalib) {
      if (type == std::string("PedestalNoise")) {
        if (o2::tpc::qc::helpers::newZSCalib(*(mRefPedestal.get()), *(mRefNoise.get()), calMap["Pedestals"])) {
          mRefPedestal.reset(nullptr);
          mRefPedestal = std::make_unique<o2::tpc::CalDet<float>>(calMap["Pedestals"]);
          ILOG(Info, Support) << "New reference pedestal file set!" << ENDM;
          mRefNoise.reset(nullptr);
          mRefNoise = std::make_unique<o2::tpc::CalDet<float>>(calMap["Noise"]);
          ILOG(Info, Support) << "New reference noise file set!" << ENDM;

          for (const auto& canvasVec : mCalDetCanvasVec) {
            for (auto& canvas : canvasVec) {
              if (canvas->GetName() == std::string("c_Sides_Pedestals")) {
                canvas->cd(3);
                mNewZSCalibMsg->Draw();
              }
            }
          }
        }
      }
    }
  }

  for (const auto& type : mOutputList) {
    auto& calDet = o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<o2::tpc::CalDet<float>>(fmt::format("TPC/Calib/{}", type).data(),
                                                                                                      mTimestamps.size() > 0 ? mTimestamps.at(calDetIter) : -1,
                                                                                                      mLookupMaps.size() > 1 ? mLookupMaps.at(calDetIter) : mLookupMaps.at(0));
    auto vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;
  }
}

void CalDetPublisher::finalize(Trigger t, framework::ServiceRegistryRef)
{
  for (const auto& calDetCanvasVec : mCalDetCanvasVec) {
    for (const auto& canvas : calDetCanvasVec) {
      getObjectsManager()->stopPublishing(canvas.get());
    }
  }
  mCalDetCanvasVec.clear();
}

} // namespace o2::quality_control_modules::tpc
