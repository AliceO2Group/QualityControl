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
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"
#include "TPCQC/Helpers.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/CalDetPublisher.h"
#include "TPC/Utility.h"

//root includes
#include "TCanvas.h"
#include "TPaveText.h"

#include <fmt/format.h>
#include <algorithm>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void CalDetPublisher::configure(std::string name, const boost::property_tree::ptree& config)
{
  for (const auto& output : config.get_child("qc.postprocessing." + name + ".outputCalPadMaps")) {
    mOutputListMap.emplace_back(output.second.data());
  }

  for (const auto& output : config.get_child("qc.postprocessing." + name + ".outputCalPads")) {
    mOutputList.emplace_back(output.second.data());
  }

  for (const auto& timestamp : config.get_child("qc.postprocessing." + name + ".timestamps")) {
    mTimestamps.emplace_back(std::stol(timestamp.second.data()));
  }

  std::vector<std::string> keyVec{};
  std::vector<std::string> valueVec{};
  for (const auto& data : config.get_child("qc.postprocessing." + name + ".lookupMetaData")) {
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
      throw std::runtime_error("Number of keys and values for lookupMetaData are not matching");
    }
    keyVec.clear();
    valueVec.clear();
  }

  for (const auto& data : config.get_child("qc.postprocessing." + name + ".storeMetaData")) {
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
      throw std::runtime_error("Number of keys and values for storeMetaData are not matching");
    }
    keyVec.clear();
    valueVec.clear();
  }

  if ((mTimestamps.size() != mOutputList.size()) && (mTimestamps.size() > 0)) {
    throw std::runtime_error("You need to set a timestamp for every CalPad object or none at all");
  }

  /// This needs to be put in when Pedestal and Noise are stored as unordered_map
  //if(std::find(mOutputListMap.begin(), mOutputListMap.end(), "PedestalNoise") != mOutputListMap.end()) { mCheckZSPrereq = true; }

  for (const auto& entry : config.get_child("qc.postprocessing." + name + ".histogramRanges")) {
    for (const auto& type : entry.second) {
      for (const auto& value : type.second) {
        mRanges[type.first].emplace_back(std::stof(value.second.data()));
      }
    }
  }

  std::string checkZSCalibration = config.get<std::string>("qc.postprocessing." + name + ".checkZSCalibration.check");
  if (checkZSCalibration == "true" && mCheckZSPrereq == true) {
    mCheckZSCalib = true;
    /// This needs to be put in when Pedestal and Noise are stored as unordered_map
    //mInitRefCalibTimestamp = std::stol(config.get<std::string>("qc.postprocessing." + name + ".checkZSCalibration.initRefCalibTimestamp"));
    /// This will be removed when Pedestal and Noise are stored in a unordered_map
    /// ===================================================================================================
    mInitRefPedestalTimestamp = std::stol(config.get<std::string>("qc.postprocessing." + name + ".checkZSCalibration.initRefPedestalTimestamp"));
    mInitRefNoiseTimestamp = std::stol(config.get<std::string>("qc.postprocessing." + name + ".checkZSCalibration.initRefNoiseTimestamp"));
    /// ===================================================================================================
  } else if (checkZSCalibration == "false") {
    mCheckZSCalib = false;
  } else if (checkZSCalibration == "true" && mCheckZSPrereq == false) {
    throw std::runtime_error("'Pedestal' and 'Noise' need to be in the 'outputList' to make the Zero Suppression calibration check");
  } else {
    throw std::runtime_error("No valid value for 'checkZSCalibration.check' set. Has to be 'true' or 'false'");
  }
}

void CalDetPublisher::initialize(Trigger, framework::ServiceRegistry&)
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
    /// This needs to be put in when Pedestal and Noise are stored as unordered_map
    /*auto& calMap = o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<std::unordered_map<std::string,o2::tpc::CalDet<float>>>("TPC/Calib/PutProperPathHere",
                                                                                                                                        mInitRefCalibTimestamp,
                                                                                                                                        std::map<std::string, std::string>());
    mRefPedestal = std::make_unique<o2::tpc::CalDet<float>>(calMap["Pedestal"]);
    mRefNoise = std::make_unique<o2::tpc::CalDet<float>>(calMap["Noise"]);*/

    /// This will be removed when Pedestal and Noise are stored in a unordered_map
    /// ===================================================================================================
    mRefPedestal = std::make_unique<o2::tpc::CalDet<float>>(o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<o2::tpc::CalDet<float>>("TPC/Calib/Pedestal",
                                                                                                                                               mInitRefPedestalTimestamp,
                                                                                                                                               std::map<std::string, std::string>()));
    mRefNoise = std::make_unique<o2::tpc::CalDet<float>>(o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<o2::tpc::CalDet<float>>("TPC/Calib/Noise",
                                                                                                                                            mInitRefPedestalTimestamp,
                                                                                                                                            std::map<std::string, std::string>()));
    /// ===================================================================================================

    mNewZSCalibMsg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    mNewZSCalibMsg->AddText("Upload new calib data for ZS!");
    mNewZSCalibMsg->SetFillColor(kRed);
  }
}

void CalDetPublisher::update(Trigger t, framework::ServiceRegistry&)
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

    /// This needs to be put in when Pedestal and Noise are stored as unordered_map
    /*if (mCheckZSCalib) {
      if (type == std::string("CalibVec")) {
        if (o2::tpc::qc::helpers::newZSCalib(mRefPedestal.get(), mRefNoise.get(), &calMap["Pedestal"])) {
          mRefPedestal.reset(nullptr);
          mRefPedestal = std::make_unique<o2::tpc::CalDet<float>>(calMap["Pedestal"]);
          ILOG(Info, Support) << "New reference pedestal file set!" << ENDM;
          mRefNoise.reset(nullptr);
          mRefNoise = std::make_unique<o2::tpc::CalDet<float>>(calMap["Noise"]);
          ILOG(Info, Support) << "New reference noise file set!" << ENDM;

          for (const auto& canvasVec : mCalDetCanvasVec) {
            for (auto& canvas : canvasVec) {
              if (canvas->GetName() == std::string("c_Sides_Pedestal")) {
                canvas->cd(3);
                mNewZSCalibMsg->Draw();
              }
            }
          }
        }
      }
    }*/
  }

  for (const auto& type : mOutputList) {
    auto& calDet = o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<o2::tpc::CalDet<float>>(fmt::format("TPC/Calib/{}", type).data(),
                                                                                                      mTimestamps.size() > 0 ? mTimestamps.at(calDetIter) : -1,
                                                                                                      mLookupMaps.size() > 1 ? mLookupMaps.at(calDetIter) : mLookupMaps.at(0));
    auto vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;

    /// This will be removed when Pedestal and Noise are stored in a unordered_map
    /// ===================================================================================================
    if (mCheckZSCalib) {
      bool setNewRefNoise = false;
      if (type == std::string("Pedestal")) {
        if (o2::tpc::qc::helpers::newZSCalib(*(mRefPedestal.get()), *(mRefNoise.get()), calDet)) {
          setNewRefNoise = true;
          mRefPedestal.reset(nullptr);
          mRefPedestal = std::make_unique<o2::tpc::CalDet<float>>(calDet);
          ILOG(Info, Support) << "New reference pedestal file set!" << ENDM;
        }
      }
      if (type == std::string("Noise") && setNewRefNoise) {
        mRefNoise.reset(nullptr);
        mRefNoise = std::make_unique<o2::tpc::CalDet<float>>(calDet);
        ILOG(Info, Support) << "New reference noise file set!" << ENDM;
        for (const auto& canvasVec : mCalDetCanvasVec) {
          for (const auto& canvas : canvasVec) {
            if (canvas->GetName() == std::string("c_Sides_Pedestal")) {
              canvas->cd(3);
              mNewZSCalibMsg->Draw();
            }
          }
        }
      }
    }
    /// ===================================================================================================
  }
}

void CalDetPublisher::finalize(Trigger, framework::ServiceRegistry&)
{
  for (const auto& calDetCanvasVec : mCalDetCanvasVec) {
    for (const auto& canvas : calDetCanvasVec) {
      getObjectsManager()->stopPublishing(canvas.get());
    }
  }
}

} // namespace o2::quality_control_modules::tpc
