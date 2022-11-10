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
/// \file   SACs.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "TPCBase/CDBInterface.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/SACs.h"
#include "TPC/Utility.h"

// root includes
#include "TCanvas.h"

#include <fmt/format.h>
#include <boost/optional/optional.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::tpc;

namespace o2::quality_control_modules::tpc
{

void SACs::configure(const boost::property_tree::ptree& config)
{
  auto& id = getID();
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

  for (const auto& entry : config.get_child("qc.postprocessing." + id + ".timestamps")) {
    for (const auto& type : entry.second) {
      mTimestamps[type.first] = std::stol(type.second.data());
    }
  }

  mHost = config.get<std::string>("qc.postprocessing." + id + ".dataSourceURL");

  boost::optional<const boost::property_tree::ptree&> doLatestExists = config.get_child_optional("qc.postprocessing." + id + ".doLatest");
  if (doLatestExists) {
    auto doLatest = config.get<std::string>("qc.postprocessing." + id + ".doLatest");
    if (doLatest == "1" || doLatest == "true" || doLatest == "True" || doLatest == "TRUE" || doLatest == "yes") {
      mDoLatest = true;
    } else if (doLatest == "0" || doLatest == "false" || doLatest == "False" || doLatest == "FALSE" || doLatest == "no") {
      mDoLatest = false;
    } else {
      mDoLatest = false;
      ILOG(Warning, Support) << "No valid input for 'doLatest'. Using default value 'false'." << ENDM;
    }
  } else {
    mDoLatest = false;
    ILOG(Warning, Support) << "Option 'doLatest' is missing. Using default value 'false'." << ENDM;
  }
}

void SACs::initialize(Trigger, framework::ServiceRegistryRef)
{
  mCdbApi.init(mHost);

  mSACZeroSides = std::make_unique<TCanvas>("c_sides_SACZero");
  mSACDeltaSides = std::make_unique<TCanvas>("c_sides_SACDelta");
  mSACOneSides = std::make_unique<TCanvas>("c_sides_SACOne");
  mFourierCoeffsA = std::make_unique<TCanvas>("c_FourierCoefficients_1D_ASide");
  mFourierCoeffsC = std::make_unique<TCanvas>("c_FourierCoefficients_1D_CSide");

  getObjectsManager()->startPublishing(mSACZeroSides.get());
  getObjectsManager()->startPublishing(mSACDeltaSides.get());
  getObjectsManager()->startPublishing(mSACOneSides.get());
  getObjectsManager()->startPublishing(mFourierCoeffsA.get());
  getObjectsManager()->startPublishing(mFourierCoeffsC.get());
}

void SACs::update(Trigger, framework::ServiceRegistryRef)
{
  mSACZeroSides.get()->Clear();
  mSACDeltaSides.get()->Clear();
  mSACOneSides.get()->Clear();
  mFourierCoeffsA.get()->Clear();
  mFourierCoeffsC.get()->Clear();

  o2::tpc::SACZero* sacZero = nullptr;
  o2::tpc::SACDelta<unsigned char>* sacDelta = nullptr;
  o2::tpc::SACOne* sacOne = nullptr;
  o2::tpc::FourierCoeffSAC* sacFFT = nullptr;

  if (mDoLatest) {
    std::vector<long> availableTimestampsSACZero = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalSAC0), 1, mTimestamps["SACZero"]);
    std::vector<long> availableTimestampsSACDelta = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalSACDelta), 1, mTimestamps["SACDelta"]);
    std::vector<long> availableTimestampsSACOne = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalSAC1), 1, mTimestamps["SACOne"]);
    std::vector<long> availableTimestampsSACFourierCoeffs = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalSACFourier), 1, mTimestamps["SACFourierCoeffs"]);

    sacZero = mCdbApi.retrieveFromTFileAny<SACZero>(CDBTypeMap.at(CDBType::CalSAC0), std::map<std::string, std::string>{}, availableTimestampsSACZero[0]);
    sacDelta = mCdbApi.retrieveFromTFileAny<SACDelta<unsigned char>>(CDBTypeMap.at(CDBType::CalSACDelta), std::map<std::string, std::string>{}, availableTimestampsSACDelta[0]);
    sacOne = mCdbApi.retrieveFromTFileAny<SACOne>(CDBTypeMap.at(CDBType::CalSAC1), std::map<std::string, std::string>{}, availableTimestampsSACOne[0]);
    sacFFT = mCdbApi.retrieveFromTFileAny<FourierCoeffSAC>(CDBTypeMap.at(CDBType::CalSACFourier), std::map<std::string, std::string>{}, availableTimestampsSACFourierCoeffs[0]);
  } else {
    sacZero = mCdbApi.retrieveFromTFileAny<SACZero>(CDBTypeMap.at(CDBType::CalSAC0), std::map<std::string, std::string>{}, mTimestamps["SACZero"]);
    sacDelta = mCdbApi.retrieveFromTFileAny<SACDelta<unsigned char>>(CDBTypeMap.at(CDBType::CalSACDelta), std::map<std::string, std::string>{}, mTimestamps["SACDelta"]);
    sacOne = mCdbApi.retrieveFromTFileAny<SACOne>(CDBTypeMap.at(CDBType::CalSAC1), std::map<std::string, std::string>{}, mTimestamps["SACOne"]);
    sacFFT = mCdbApi.retrieveFromTFileAny<FourierCoeffSAC>(CDBTypeMap.at(CDBType::CalSACFourier), std::map<std::string, std::string>{}, mTimestamps["SACFourierCoeffs"]);
  }

  if (sacZero) {
    mSACs.setSACZero(sacZero);
    mSACs.drawSACTypeSides(o2::tpc::SACType::IDCZero, 0, mRanges["SACZero"].at(1), mRanges["SACZero"].at(2), mSACZeroSides.get());
  }
  if (sacDelta) {
    mSACs.setSACDelta(sacDelta);
    mSACs.drawSACTypeSides(o2::tpc::SACType::IDCDelta, 0, mRanges["SACDelta"].at(1), mRanges["SACDelta"].at(2), mSACDeltaSides.get());
  }
  if (sacOne) {
    mSACs.setSACOne(sacOne, Side::A);
    mSACs.setSACOne(sacOne, Side::C);
    mSACs.drawSACOneCanvas(mRanges["SACOne"].at(0), mRanges["SACOne"].at(1), mRanges["SACOne"].at(2), 0, mSACOneSides.get());
  }
  if (sacFFT) {
    mSACs.setFourierCoeffSAC(sacFFT);
    mSACs.drawFourierCoeffSAC(Side::A, mRanges["SACFourierCoeffs"].at(0), mRanges["SACFourierCoeffs"].at(1), mRanges["SACFourierCoeffs"].at(2), mFourierCoeffsA.get());
    mSACs.drawFourierCoeffSAC(Side::C, mRanges["SACFourierCoeffs"].at(0), mRanges["SACFourierCoeffs"].at(1), mRanges["SACFourierCoeffs"].at(2), mFourierCoeffsC.get());
  }

  delete sacZero;
  delete sacDelta;
  delete sacOne;
  delete sacFFT;

  mSACs.setSACZero(nullptr);
  mSACs.setSACDelta<unsigned char>(nullptr);
  mSACs.setSACOne(nullptr, Side::A);
  mSACs.setSACOne(nullptr, Side::C);
  mSACs.setFourierCoeffSAC(nullptr);
}

void SACs::finalize(Trigger, framework::ServiceRegistryRef)
{
  getObjectsManager()->stopPublishing(mSACZeroSides.get());
  getObjectsManager()->stopPublishing(mSACOneSides.get());
  getObjectsManager()->stopPublishing(mSACDeltaSides.get());
  getObjectsManager()->stopPublishing(mFourierCoeffsA.get());
  getObjectsManager()->stopPublishing(mFourierCoeffsC.get());
}

} // namespace o2::quality_control_modules::tpc