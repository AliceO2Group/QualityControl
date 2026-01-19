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
/// \file   IDCsVsSACs.cxx
/// \author Bhawani Singh
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
#include "TPC/IDCsVsSACs.h"

// root includes
#include "TCanvas.h"

#include <fmt/format.h>
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::tpc;

namespace o2::quality_control_modules::tpc
{

void IDCsVsSACs::configure(const boost::property_tree::ptree& config)
{
  std::vector<std::string> keyVec{};
  std::vector<std::string> valueVec{};
  auto& id = getID();
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
}

void IDCsVsSACs::initialize(Trigger, framework::ServiceRegistryRef)
{
  mCdbApi.init(mHost);
  mCompareIDC0andSAC0 = std::make_unique<TCanvas>("c_mCompareIDC0andSAC0");
  getObjectsManager()->startPublishing(mCompareIDC0andSAC0.get());
}

void IDCsVsSACs::update(Trigger, framework::ServiceRegistryRef)
{
  mCompareIDC0andSAC0.get()->Clear();
  auto idcZeroA = mCdbApi.retrieveFromTFileAny<IDCZero>(CDBTypeMap.at(CDBType::CalIDC0A), std::map<std::string, std::string>{}, mTimestamps["IDCZero"]);
  auto idcZeroC = mCdbApi.retrieveFromTFileAny<IDCZero>(CDBTypeMap.at(CDBType::CalIDC0C), std::map<std::string, std::string>{}, mTimestamps["IDCZero"]);
  mCCDBHelper.setIDCZero(idcZeroA, Side::A);
  mCCDBHelper.setIDCZero(idcZeroC, Side::C);
  mCCDBHelper.setIDCZeroScale(true);

  auto sacZero = mCdbApi.retrieveFromTFileAny<SACZero>(CDBTypeMap.at(CDBType::CalSAC0), std::map<std::string, std::string>{}, mTimestamps["SACZero"]);
  mSACs.setSACZero(sacZero);
  mIDCsVsSACs = o2::tpc::qc::IDCsVsSACs(&mCCDBHelper, &mSACs);
  mIDCsVsSACs.drawComparisionSACandIDCZero(mCompareIDC0andSAC0.get(), mRanges["IDCZero"].at(0), mRanges["IDCZero"].at(1), mRanges["IDCZero"].at(2), mRanges["SACZero"].at(0), mRanges["SACZero"].at(1), mRanges["SACZero"].at(2));
  delete idcZeroA;
  delete idcZeroC;
  delete sacZero;

  mCCDBHelper.setIDCZero(nullptr, Side::A);
  mCCDBHelper.setIDCZero(nullptr, Side::C);
  mSACs.setSACZero(nullptr);
}

void IDCsVsSACs::finalize(Trigger, framework::ServiceRegistryRef)
{
  getObjectsManager()->stopPublishing(mCompareIDC0andSAC0.get());
  mCompareIDC0andSAC0.reset();
}

} // namespace o2::quality_control_modules::tpc
