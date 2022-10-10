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
/// \file   IDCs.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "TPCBase/CDBInterface.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/IDCs.h"

// root includes
#include "TCanvas.h"

#include <fmt/format.h>

using namespace o2::quality_control::postprocessing;
using namespace o2::tpc;

namespace o2::quality_control_modules::tpc
{

void IDCs::configure(std::string name, const boost::property_tree::ptree& config)
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
      ILOG(Error, Support) << "Number of keys and values for lookupMetaData are not matching" << ENDM;
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
      ILOG(Error, Support) << "Number of keys and values for storeMetaData are not matching" << ENDM;
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

  mHost = config.get<std::string>("qc.postprocessing." + name + ".dataSourceURL");
}

void IDCs::initialize(Trigger, framework::ServiceRegistryRef)
{
  mCdbApi.init(mHost);

  mIDCZeroRadialProf = std::make_unique<TCanvas>("c_sides_IDC0_radialProfile");
  mIDCZeroStacksA = std::make_unique<TCanvas>("c_GEMStacks_IDC0_1D_ASide");
  mIDCZeroStacksC = std::make_unique<TCanvas>("c_GEMStacks_IDC0_1D_CSide");

  mIDCDeltaStacksA = std::make_unique<TCanvas>("c_GEMStacks_IDCDelta_1D_ASide");
  mIDCDeltaStacksC = std::make_unique<TCanvas>("c_GEMStacks_IDCDelta_1D_CSide");

  mIDCOneSides1D = std::make_unique<TCanvas>("c_sides_IDC1_1D");

  mFourierCoeffsA = std::make_unique<TCanvas>("c_FourierCoefficients_1D_ASide");
  mFourierCoeffsC = std::make_unique<TCanvas>("c_FourierCoefficients_1D_CSide");

  getObjectsManager()->startPublishing(mIDCZeroRadialProf.get());
  getObjectsManager()->startPublishing(mIDCZeroStacksA.get());
  getObjectsManager()->startPublishing(mIDCZeroStacksC.get());

  getObjectsManager()->startPublishing(mIDCDeltaStacksA.get());
  getObjectsManager()->startPublishing(mIDCDeltaStacksC.get());

  getObjectsManager()->startPublishing(mIDCOneSides1D.get());

  getObjectsManager()->startPublishing(mFourierCoeffsA.get());
  getObjectsManager()->startPublishing(mFourierCoeffsC.get());
}

void IDCs::update(Trigger, framework::ServiceRegistryRef)
{
  auto idcZeroA = mCdbApi.retrieveFromTFileAny<IDCZero>(CDBTypeMap.at(CDBType::CalIDC0A), std::map<std::string, std::string>{}, mTimestamps["IDCZero"]);
  auto idcZeroC = mCdbApi.retrieveFromTFileAny<IDCZero>(CDBTypeMap.at(CDBType::CalIDC0C), std::map<std::string, std::string>{}, mTimestamps["IDCZero"]);
  auto idcDeltaA = mCdbApi.retrieveFromTFileAny<IDCDelta<unsigned char>>(CDBTypeMap.at(CDBType::CalIDCDeltaA), std::map<std::string, std::string>{}, mTimestamps["IDCDelta"]);
  auto idcDeltaC = mCdbApi.retrieveFromTFileAny<IDCDelta<unsigned char>>(CDBTypeMap.at(CDBType::CalIDCDeltaC), std::map<std::string, std::string>{}, mTimestamps["IDCDelta"]);
  auto idcOneA = mCdbApi.retrieveFromTFileAny<IDCOne>(CDBTypeMap.at(CDBType::CalIDC1A), std::map<std::string, std::string>{}, mTimestamps["IDCOne"]);
  auto idcOneC = mCdbApi.retrieveFromTFileAny<IDCOne>(CDBTypeMap.at(CDBType::CalIDC1C), std::map<std::string, std::string>{}, mTimestamps["IDCOne"]);
  auto idcFFTA = mCdbApi.retrieveFromTFileAny<FourierCoeff>(CDBTypeMap.at(CDBType::CalIDCFourierA), std::map<std::string, std::string>{}, mTimestamps["FourierCoeffs"]);
  auto idcFFTC = mCdbApi.retrieveFromTFileAny<FourierCoeff>(CDBTypeMap.at(CDBType::CalIDCFourierC), std::map<std::string, std::string>{}, mTimestamps["FourierCoeffs"]);

  mCCDBHelper.setIDCZero(idcZeroA, Side::A);
  mCCDBHelper.setIDCZero(idcZeroC, Side::C);
  mCCDBHelper.setIDCDelta(idcDeltaA, Side::A);
  mCCDBHelper.setIDCDelta(idcDeltaC, Side::C);
  mCCDBHelper.setIDCOne(idcOneA, Side::A);
  mCCDBHelper.setIDCOne(idcOneC, Side::C);
  mCCDBHelper.setFourierCoeffs(idcFFTA, Side::A);
  mCCDBHelper.setFourierCoeffs(idcFFTC, Side::C);

  if (idcZeroA) {
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCZeroStacksA.get(), Side::A, "IDC0", mRanges["IDCZero"].at(0), mRanges["IDCZero"].at(1), mRanges["IDCZero"].at(2));
  }

  if (idcZeroC) {
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCZeroStacksC.get(), Side::C, "IDC0", mRanges["IDCZero"].at(0), mRanges["IDCZero"].at(1), mRanges["IDCZero"].at(2));
  }

  if (idcZeroA && idcZeroC) {
    mCCDBHelper.drawIDCZeroRadialProfile(mIDCZeroRadialProf.get(), mRanges["IDCZero"].at(0), mRanges["IDCZero"].at(1), mRanges["IDCZero"].at(2));
  }

  if (idcDeltaA) {
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCDeltaStacksA.get(), Side::A, "IDCDelta", mRanges["IDCDelta"].at(0), mRanges["IDCDelta"].at(1), mRanges["IDCDelta"].at(2));
  }

  if (idcDeltaC) {
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCDeltaStacksC.get(), Side::C, "IDCDelta", mRanges["IDCDelta"].at(0), mRanges["IDCDelta"].at(1), mRanges["IDCDelta"].at(2));
  }

  if (idcOneA && idcOneC) {
    mCCDBHelper.drawIDCOneCanvas(mIDCOneSides1D.get(), mRanges["IDCOne"].at(0), mRanges["IDCOne"].at(1), mRanges["IDCOne"].at(2));
  }

  if (idcFFTA) {
    mCCDBHelper.drawFourierCoeff(mFourierCoeffsA.get(), Side::A, mRanges["FourierCoeffs"].at(0), mRanges["FourierCoeffs"].at(1), mRanges["FourierCoeffs"].at(2));
  }

  if (idcFFTC) {
    mCCDBHelper.drawFourierCoeff(mFourierCoeffsC.get(), Side::C, mRanges["FourierCoeffs"].at(0), mRanges["FourierCoeffs"].at(1), mRanges["FourierCoeffs"].at(2));
  }

  delete idcZeroA;
  delete idcZeroC;
  delete idcDeltaA;
  delete idcDeltaC;
  delete idcOneA;
  delete idcOneC;
  delete idcFFTA;
  delete idcFFTC;

  mCCDBHelper.setIDCZero(nullptr, Side::A);
  mCCDBHelper.setIDCZero(nullptr, Side::C);
  mCCDBHelper.setIDCDelta(nullptr, Side::A);
  mCCDBHelper.setIDCDelta(nullptr, Side::C);
  mCCDBHelper.setIDCOne(nullptr, Side::A);
  mCCDBHelper.setIDCOne(nullptr, Side::C);
  mCCDBHelper.setFourierCoeffs(nullptr, Side::A);
  mCCDBHelper.setFourierCoeffs(nullptr, Side::C);
}

void IDCs::finalize(Trigger, framework::ServiceRegistryRef)
{
  getObjectsManager()->stopPublishing(mIDCZeroRadialProf.get());
  getObjectsManager()->stopPublishing(mIDCZeroStacksA.get());
  getObjectsManager()->stopPublishing(mIDCZeroStacksC.get());

  getObjectsManager()->stopPublishing(mIDCDeltaStacksA.get());
  getObjectsManager()->stopPublishing(mIDCDeltaStacksC.get());

  getObjectsManager()->stopPublishing(mIDCOneSides1D.get());

  getObjectsManager()->stopPublishing(mFourierCoeffsA.get());
  getObjectsManager()->stopPublishing(mFourierCoeffsC.get());
}

} // namespace o2::quality_control_modules::tpc
