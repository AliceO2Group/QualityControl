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

// root includes
#include "TCanvas.h"

#include <fmt/format.h>

using namespace o2::quality_control::postprocessing;
using namespace o2::tpc;

namespace o2::quality_control_modules::tpc
{

void SACs::configure(std::string name, const boost::property_tree::ptree& config)
{
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

  for (const auto& entry : config.get_child("qc.postprocessing." + name + ".histogramRanges")) {
    for (const auto& type : entry.second) {
      for (const auto& value : type.second) {
        mRanges[type.first].emplace_back(std::stof(value.second.data()));
      }
    }
  }

  for (const auto& entry : config.get_child("qc.postprocessing." + name + ".timestamps")) {
    for (const auto& type : entry.second) {
      mTimestamps[type.first] = std::stol(type.second.data());
    }
  }
  mHost = config.get<std::string>("qc.config.conditionDB.url");
}

void SACs::initialize(Trigger, framework::ServiceRegistry&)
{
  mCdbApi.init(mHost);
  mSACZeroSides = std::make_unique<TCanvas>("c_sides_SACZero");
  getObjectsManager()->startPublishing(mSACZeroSides.get());

  mSACDeltaSides = std::make_unique<TCanvas>("c_sides_SACDelta");
  getObjectsManager()->startPublishing(mSACDeltaSides.get());

  mSACOneSides = std::make_unique<TCanvas>("c_sides_SACOne");
  getObjectsManager()->startPublishing(mSACOneSides.get());
  mFourierCoeffsA = std::make_unique<TCanvas>("c_FourierCoefficients_1D_ASide");
  getObjectsManager()->startPublishing(mFourierCoeffsA.get());
  mFourierCoeffsC = std::make_unique<TCanvas>("c_FourierCoefficients_1D_CSide");
  getObjectsManager()->startPublishing(mFourierCoeffsC.get());
}

void SACs::update(Trigger, framework::ServiceRegistry&)
{
  auto sacZero = mCdbApi.retrieveFromTFileAny<SACZero>(CDBTypeMap.at(CDBType::CalSAC0), std::map<std::string, std::string>{}, mTimestamps["SACZero"]);
  auto sacDelta = mCdbApi.retrieveFromTFileAny<SACDelta<unsigned char>>(CDBTypeMap.at(CDBType::CalSACDelta), std::map<std::string, std::string>{}, mTimestamps["SACDelta"]);

  auto sacOneA = mCdbApi.retrieveFromTFileAny<SACOne>(CDBTypeMap.at(CDBType::CalSAC1), std::map<std::string, std::string>{}, mTimestamps["SACOne"]);
  auto sacOneC = mCdbApi.retrieveFromTFileAny<SACOne>(CDBTypeMap.at(CDBType::CalSAC1), std::map<std::string, std::string>{}, mTimestamps["SACOne"]);

  auto sacFFTA = mCdbApi.retrieveFromTFileAny<FourierCoeffSAC>(CDBTypeMap.at(CDBType::CalSACFourier), std::map<std::string, std::string>{}, mTimestamps["SACFourierCoeffs"]);
  auto sacFFTC = mCdbApi.retrieveFromTFileAny<FourierCoeffSAC>(CDBTypeMap.at(CDBType::CalSACFourier), std::map<std::string, std::string>{}, mTimestamps["SACFourierCoeffs"]);

  mSACs.setSACZero(*sacZero);
  mSACs.setSACDelta(*sacDelta);
  mSACs.setSACOne(sacOneA, Side::A);
  mSACs.setSACOne(sacOneC, Side::C);

  mSACs.setFourierCoeffSAC(sacFFTA);
  mSACs.setFourierCoeffSAC(sacFFTC);

  mSACs.drawSACTypeSides(o2::tpc::SACType::IDCZero, 0, mRanges["SACZero"].at(1), mRanges["SACZero"].at(2), mSACZeroSides.get());
  mSACs.drawSACTypeSides(o2::tpc::SACType::IDCDelta, 0, mRanges["SACDelta"].at(1), mRanges["SACDelta"].at(2), mSACDeltaSides.get());

  mSACs.drawSACOneCanvas(mSACOneSides.get(), mRanges["SACOne"].at(0), mRanges["SACOne"].at(1), mRanges["SACOne"].at(2), 0);

  mSACs.drawFourierCoeffSAC(mFourierCoeffsA.get(), Side::A, mRanges["SACFourierCoeffs"].at(0), mRanges["SACFourierCoeffs"].at(1), mRanges["SACFourierCoeffs"].at(2));
  mSACs.drawFourierCoeffSAC(mFourierCoeffsC.get(), Side::C, mRanges["SACFourierCoeffs"].at(0), mRanges["SACFourierCoeffs"].at(1), mRanges["SACFourierCoeffs"].at(2));

  delete sacOneA;
  delete sacOneC;

  delete sacFFTA;
  delete sacFFTC;
  mSACs.setSACOne(nullptr, Side::A);
  mSACs.setSACOne(nullptr, Side::C);
  mSACs.setFourierCoeffSAC(nullptr);
}

void SACs::finalize(Trigger, framework::ServiceRegistry&)
{
  getObjectsManager()->stopPublishing(mSACZeroSides.get());
  getObjectsManager()->stopPublishing(mSACOneSides.get());
  getObjectsManager()->stopPublishing(mSACDeltaSides.get());
  getObjectsManager()->stopPublishing(mFourierCoeffsA.get());
  getObjectsManager()->stopPublishing(mFourierCoeffsC.get());
}

} // namespace o2::quality_control_modules::tpc