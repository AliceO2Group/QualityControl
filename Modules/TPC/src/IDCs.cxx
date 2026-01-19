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
#include "TPCBase/CalArray.h"
#if __has_include("TPCBase/Painter.h")
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"
#else
#include "TPCBaseRecSim/Painter.h"
#include "TPCBaseRecSim/CDBInterface.h"
#endif
#include "TPCBase/CalDet.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/IDCs.h"
#include "TPC/Utility.h"

// root includes
#include "TCanvas.h"

#include <fmt/format.h>
#include <boost/optional/optional.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::tpc;

namespace o2::quality_control_modules::tpc
{

void IDCs::configure(const boost::property_tree::ptree& config)
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

  mDoIDCDelta = getPropertyBool(config, id, "doIDCDelta");
  mDoIDC1 = getPropertyBool(config, id, "doIDC1");
  mDoFourier = getPropertyBool(config, id, "doFourier");
}

void IDCs::initialize(Trigger, framework::ServiceRegistryRef)
{
  mCdbApi.init(mHost);

  mIDCZeroScale.reset();
  mIDCZerOverview.reset();
  mIDCZeroRadialProf.reset();
  mIDCZeroStacksA.reset();
  mIDCZeroStacksC.reset();

  mIDCZeroScale = std::make_unique<TCanvas>("c_sides_IDC0_scale");
  mIDCZerOverview = std::make_unique<TCanvas>("c_sides_IDC0_overview");
  mIDCZeroRadialProf = std::make_unique<TCanvas>("c_sides_IDC0_radialProfile");
  mIDCZeroStacksA = std::make_unique<TCanvas>("c_GEMStacks_IDC0_1D_ASide");
  mIDCZeroStacksC = std::make_unique<TCanvas>("c_GEMStacks_IDC0_1D_CSide");

  getObjectsManager()->startPublishing(mIDCZeroScale.get());
  getObjectsManager()->startPublishing(mIDCZerOverview.get());
  getObjectsManager()->startPublishing(mIDCZeroRadialProf.get());
  getObjectsManager()->startPublishing(mIDCZeroStacksA.get());
  getObjectsManager()->startPublishing(mIDCZeroStacksC.get());

  if (mDoIDCDelta) {
    mIDCDeltaStacksA.reset();
    mIDCDeltaStacksC.reset();
    mIDCDeltaStacksA = std::make_unique<TCanvas>("c_GEMStacks_IDCDelta_1D_ASide");
    mIDCDeltaStacksC = std::make_unique<TCanvas>("c_GEMStacks_IDCDelta_1D_CSide");
    getObjectsManager()->startPublishing(mIDCDeltaStacksA.get());
    getObjectsManager()->startPublishing(mIDCDeltaStacksC.get());
  }

  if (mDoIDC1) {
    mIDCOneSides1D.reset();
    mIDCOneSides1D = std::make_unique<TCanvas>("c_sides_IDC1_1D");
    getObjectsManager()->startPublishing(mIDCOneSides1D.get());
  }

  if (mDoFourier) {
    mFourierCoeffsA.reset();
    mFourierCoeffsC.reset();
    mFourierCoeffsA = std::make_unique<TCanvas>("c_FourierCoefficients_1D_ASide");
    mFourierCoeffsC = std::make_unique<TCanvas>("c_FourierCoefficients_1D_CSide");
    getObjectsManager()->startPublishing(mFourierCoeffsA.get());
    getObjectsManager()->startPublishing(mFourierCoeffsC.get());
  }
}

void IDCs::update(Trigger, framework::ServiceRegistryRef)
{
  std::vector<long> availableTimestampsIDCZeroA = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDC0A), 1, mTimestamps["IDCZero"]);
  std::vector<long> availableTimestampsIDCZeroC = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDC0C), 1, mTimestamps["IDCZero"]);
  std::vector<long> availableTimestampsIDCDeltaA{ 0 };
  std::vector<long> availableTimestampsIDCDeltaC{ 0 };
  std::vector<long> availableTimestampsIDCOneA{ 0 };
  std::vector<long> availableTimestampsIDCOneC{ 0 };
  std::vector<long> availableTimestampsFFTA{ 0 };
  std::vector<long> availableTimestampsFFTC{ 0 };
  bool timestampFoundForIDCZero = false;
  if (availableTimestampsIDCZeroA.size() == 0 || availableTimestampsIDCZeroC.size() == 0) {
    ILOG(Warning, Support) << fmt::format("No timstemp found for '{}' produced in the last day.", "IDCZero") << ENDM;
  } else {
    timestampFoundForIDCZero = true;
  }

  bool timestampFoundForIDCDelta = false;
  if (mDoIDCDelta) {
    availableTimestampsIDCDeltaA = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDCDeltaA), 1, mTimestamps["IDCDelta"]);
    availableTimestampsIDCDeltaC = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDCDeltaC), 1, mTimestamps["IDCDelta"]);
    if (availableTimestampsIDCDeltaA.size() == 0 || availableTimestampsIDCDeltaC.size() == 0) {
      ILOG(Warning, Support) << fmt::format("No timstemp found for '{}' produced in the last day.", "IDCDelta") << ENDM;
    } else {
      timestampFoundForIDCDelta = true;
    }
    mIDCDeltaStacksA.get()->Clear();
    mIDCDeltaStacksC.get()->Clear();
  }

  bool timestampFoundForIDCOne = false;
  if (mDoIDC1) {
    availableTimestampsIDCOneA = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDC1A), 1, mTimestamps["IDCOne"]);
    availableTimestampsIDCOneC = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDC1C), 1, mTimestamps["IDCOne"]);
    if (availableTimestampsIDCOneA.size() == 0 || availableTimestampsIDCOneC.size() == 0) {
      ILOG(Warning, Support) << fmt::format("No timstemp found for '{}' produced in the last day.", "IDCOne") << ENDM;
    } else {
      timestampFoundForIDCOne = true;
    }
    mIDCOneSides1D.get()->Clear();
  }

  bool timestampFoundForFourier = false;
  if (mDoFourier) {
    availableTimestampsFFTA = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDCFourierA), 1, mTimestamps["FourierCoeffs"]);
    availableTimestampsFFTC = getDataTimestamps(mCdbApi, CDBTypeMap.at(CDBType::CalIDCFourierC), 1, mTimestamps["FourierCoeffs"]);
    if (availableTimestampsFFTA.size() == 0 || availableTimestampsFFTC.size() == 0) {
      ILOG(Warning, Support) << fmt::format("No timstemp found for '{}' produced in the last day.", "FourierCoeffs") << ENDM;
    } else {
      timestampFoundForFourier = true;
    }
    mFourierCoeffsA.get()->Clear();
    mFourierCoeffsC.get()->Clear();
  }

  mIDCZeroScale.get()->Clear();
  mIDCZerOverview.get()->Clear();
  mIDCZeroRadialProf.get()->Clear();
  mIDCZeroStacksA.get()->Clear();
  mIDCZeroStacksC.get()->Clear();

  o2::tpc::IDCZero* idcZeroA = nullptr;
  o2::tpc::IDCZero* idcZeroC = nullptr;
  o2::tpc::IDCDelta<unsigned char>* idcDeltaA = nullptr;
  o2::tpc::IDCDelta<unsigned char>* idcDeltaC = nullptr;
  o2::tpc::IDCOne* idcOneA = nullptr;
  o2::tpc::IDCOne* idcOneC = nullptr;
  o2::tpc::FourierCoeff* idcFFTA = nullptr;
  o2::tpc::FourierCoeff* idcFFTC = nullptr;

  if (timestampFoundForIDCZero) {
    idcZeroA = mCdbApi.retrieveFromTFileAny<IDCZero>(CDBTypeMap.at(CDBType::CalIDC0A), std::map<std::string, std::string>{}, availableTimestampsIDCZeroA[0]);
    idcZeroC = mCdbApi.retrieveFromTFileAny<IDCZero>(CDBTypeMap.at(CDBType::CalIDC0C), std::map<std::string, std::string>{}, availableTimestampsIDCZeroC[0]);
  }

  if (mDoIDCDelta && timestampFoundForIDCDelta) {
    idcDeltaA = mCdbApi.retrieveFromTFileAny<IDCDelta<unsigned char>>(CDBTypeMap.at(CDBType::CalIDCDeltaA), std::map<std::string, std::string>{}, availableTimestampsIDCDeltaA[0]);
    idcDeltaC = mCdbApi.retrieveFromTFileAny<IDCDelta<unsigned char>>(CDBTypeMap.at(CDBType::CalIDCDeltaC), std::map<std::string, std::string>{}, availableTimestampsIDCDeltaC[0]);
  }
  if (mDoIDC1 && timestampFoundForIDCOne) {
    idcOneA = mCdbApi.retrieveFromTFileAny<IDCOne>(CDBTypeMap.at(CDBType::CalIDC1A), std::map<std::string, std::string>{}, availableTimestampsIDCOneA[0]);
    idcOneC = mCdbApi.retrieveFromTFileAny<IDCOne>(CDBTypeMap.at(CDBType::CalIDC1C), std::map<std::string, std::string>{}, availableTimestampsIDCOneC[0]);
  }

  if (mDoFourier && timestampFoundForFourier) {
    idcFFTA = mCdbApi.retrieveFromTFileAny<FourierCoeff>(CDBTypeMap.at(CDBType::CalIDCFourierA), std::map<std::string, std::string>{}, availableTimestampsFFTA[0]);
    idcFFTC = mCdbApi.retrieveFromTFileAny<FourierCoeff>(CDBTypeMap.at(CDBType::CalIDCFourierC), std::map<std::string, std::string>{}, availableTimestampsFFTC[0]);
  }

  if (idcZeroA && idcZeroC) {
    mCCDBHelper.setIDCZero(idcZeroA, Side::A);
    mCCDBHelper.setIDCZero(idcZeroC, Side::C);
    // scale IDCZero to the sum of IDCZeros
    mCCDBHelper.setIDCZeroScale(true);
    mCCDBHelper.drawIDCZeroScale(mIDCZeroScale.get(), true);
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCZeroStacksA.get(), Side::A, "IDC0", mRanges["IDCZero"].at(0), mRanges["IDCZero"].at(1), mRanges["IDCZero"].at(2));
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCZeroStacksC.get(), Side::C, "IDC0", mRanges["IDCZero"].at(0), mRanges["IDCZero"].at(1), mRanges["IDCZero"].at(2));
    mCCDBHelper.drawIDCZeroRadialProfile(mIDCZeroRadialProf.get(), mRanges["IDCZero"].at(0), mRanges["IDCZero"].at(1), mRanges["IDCZero"].at(2));

    const auto& calDet = mCCDBHelper.getIDCZeroCalDet();
    o2::tpc::painter::draw(calDet, mRanges["IDCZeroOveview"].at(0), mRanges["IDCZeroOveview"].at(1), mRanges["IDCZeroOveview"].at(2), mIDCZerOverview.get());
  }

  if (idcDeltaA) {
    mCCDBHelper.setIDCDelta(idcDeltaA, Side::A);
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCDeltaStacksA.get(), Side::A, "IDCDelta", mRanges["IDCDelta"].at(0), mRanges["IDCDelta"].at(1), mRanges["IDCDelta"].at(2));
  }

  if (idcDeltaC) {
    mCCDBHelper.setIDCDelta(idcDeltaC, Side::C);
    mCCDBHelper.drawIDCZeroStackCanvas(mIDCDeltaStacksC.get(), Side::C, "IDCDelta", mRanges["IDCDelta"].at(0), mRanges["IDCDelta"].at(1), mRanges["IDCDelta"].at(2));
  }

  if (idcOneA && idcOneC) {
    mCCDBHelper.setIDCOne(idcOneA, Side::A);
    mCCDBHelper.setIDCOne(idcOneC, Side::C);
    mCCDBHelper.drawIDCOneCanvas(mIDCOneSides1D.get(), mRanges["IDCOne"].at(0), mRanges["IDCOne"].at(1), mRanges["IDCOne"].at(2));
  }

  if (idcFFTA) {
    mCCDBHelper.setFourierCoeffs(idcFFTA, Side::A);
    mCCDBHelper.drawFourierCoeff(mFourierCoeffsA.get(), Side::A, mRanges["FourierCoeffs"].at(0), mRanges["FourierCoeffs"].at(1), mRanges["FourierCoeffs"].at(2));
  }

  if (idcFFTC) {
    mCCDBHelper.setFourierCoeffs(idcFFTC, Side::C);
    mCCDBHelper.drawFourierCoeff(mFourierCoeffsC.get(), Side::C, mRanges["IDCZeroOveview"].at(0), mRanges["IDCZeroOveview"].at(1), mRanges["IDCZeroOveview"].at(2));
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
  getObjectsManager()->stopPublishing(mIDCZeroScale.get());
  getObjectsManager()->stopPublishing(mIDCZerOverview.get());
  getObjectsManager()->stopPublishing(mIDCZeroRadialProf.get());
  getObjectsManager()->stopPublishing(mIDCZeroStacksA.get());
  getObjectsManager()->stopPublishing(mIDCZeroStacksC.get());

  if (mDoIDC1) {
    getObjectsManager()->stopPublishing(mIDCOneSides1D.get());
  }

  if (mDoFourier) {
    getObjectsManager()->stopPublishing(mFourierCoeffsA.get());
    getObjectsManager()->stopPublishing(mFourierCoeffsC.get());
  }

  if (mDoIDCDelta) {
    getObjectsManager()->stopPublishing(mIDCDeltaStacksA.get());
    getObjectsManager()->stopPublishing(mIDCDeltaStacksC.get());
  }
}

} // namespace o2::quality_control_modules::tpc
