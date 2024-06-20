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
/// \file   PostProcHelper.h
/// \author Artur Furs afurs@cern.ch
///

#ifndef QC_MODULE_FIT_POSTPROCHELPER_H
#define QC_MODULE_FIT_POSTPROCHELPER_H

#include <string>
#include <map>

#include "CCDB/CcdbApi.h"
#include "DataFormatsParameters/GRPLHCIFData.h"
#include "QualityControl/Triggers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/PostProcessingInterface.h"

#include "FITCommon/HelperHist.h"
#include "FITCommon/HelperCommon.h"

namespace o2::quality_control_modules::fit
{

class PostProcHelper
{
 public:
  using Trigger = o2::quality_control::postprocessing::Trigger;
  void configure(const boost::property_tree::ptree& config, const char* configPath, const std::string& detName)
  {
    mCcdbUrl = config.get_child("qc.config.conditionDB.url").get_value<std::string>();
    const char* configCustom = Form("%s.custom", configPath);
    auto cfgPath = [&configCustom](const std::string& entry) {
      return Form("%s.%s", configCustom, entry.c_str());
    };
    mPathGrpLhcIf = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("pathGrpLhcIf"), "GLO/Config/GRPLHCIF");
    mPathInputQcTask = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("pathDigitQcTask"), std::string{ detName + "/MO/DigitQcTask/" });

    mNumOrbitsInTF = helper::getConfigFromPropertyTree<int>(config, cfgPath("numOrbitsInTF"), 32);
    mMetaAnchorInput = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("metaAnchorInput"), "CycleDurationNTF");
    mTimestampSource = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("timestampSource"), "metadata");
    mTimestampMetaField = helper::getConfigFromPropertyTree<std::string>(config, cfgPath("timestampMetaField"), "timestampTF");
  }

  void initialize(o2::quality_control::postprocessing::Trigger trg, framework::ServiceRegistryRef services)
  {
    mDatabase = &services.get<o2::quality_control::repository::DatabaseInterface>();
    mCcdbApi.init(mCcdbUrl);
  }
  // config fields
  std::string mPathGrpLhcIf{ "GLO/Config/GRPLHCIF" };
  std::string mPathInputQcTask{ "" };
  std::string mCcdbUrl{ "" };
  int mNumOrbitsInTF{ 32 };
  std::string mMetaAnchorInput{ "CycleDurationNTF" };
  std::string mTimestampSource{ "trigger" };
  std::string mAsynchChannelLogic{ "standard" };
  std::string mTimestampMetaField{ "timestampTF" };
  int mLowTimeThreshold{ -192 };
  int mUpTimeThreshold{ -192 };

  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  o2::ccdb::CcdbApi mCcdbApi;
  o2::parameters::GRPLHCIFData mGRPLHCIFData{};
  Trigger mCurrTrigger = Trigger(o2::quality_control::postprocessing::TriggerType::No);
  long long mTimestampAnchor{ -1 };   // timestamp for fetching entries from CCDB
  long long mCurrSampleLengthTF{ 0 }; // incomming sample length in TF units
  double mCurrSampleLengthSec{ 0. };  // incomming sample length in seconds

  void setTrigger(const o2::quality_control::postprocessing::Trigger& trg) { mCurrTrigger = trg; }
  const o2::parameters::GRPLHCIFData getGRPLHCIFData() const { return mGRPLHCIFData; }

  template <typename ObjType>
  auto getObject(const std::string& moName) const -> std::unique_ptr<ObjType>
  {
    auto mo = mDatabase->retrieveMO(mPathInputQcTask, moName, mCurrTrigger.timestamp, mCurrTrigger.activity);
    ObjType* obj = mo ? dynamic_cast<ObjType*>(mo->getObject()) : nullptr;
    if (!obj) {
      ILOG(Error) << "MO " << moName << " is NOT retrieved!" << ENDM;
    }
    return obj ? std::move(std::unique_ptr<ObjType>(dynamic_cast<ObjType*>(obj->Clone(moName.c_str())))) : nullptr;
  }

  void getMetadata()
  {
    const auto moMetadata = mDatabase->retrieveMO(mPathInputQcTask, mMetaAnchorInput, mCurrTrigger.timestamp, mCurrTrigger.activity);
    const auto hMetadata = moMetadata ? dynamic_cast<TH1*>(moMetadata->getObject()) : nullptr;
    // Getting sample length
    if (hMetadata) {
      mCurrSampleLengthTF = hMetadata->GetBinContent(1);
      mCurrSampleLengthSec = mCurrSampleLengthTF * mNumOrbitsInTF * o2::constants::lhc::LHCOrbitNS * 1e-9;
      mIsMetadataValid = true;
    } else {
      mCurrSampleLengthTF = 0;
      mCurrSampleLengthSec = 0.;
      mTimestampAnchor = -1;
      ILOG(Error) << "Cannot get anchor hist " << mMetaAnchorInput << " with required metadata" << ENDM;
      mIsMetadataValid = false;
    }
    if (mCurrSampleLengthTF) {
      mIsNonEmptySample = true;
    } else {
      mIsNonEmptySample = false;
    }
    // Getting timestamp
    if (mTimestampSource == "trigger") {
      mTimestampAnchor = mCurrTrigger.timestamp;
    } else if (mTimestampSource == "validUntil") {
      mTimestampAnchor = mCurrTrigger.activity.mValidity.getMax();
    } else if (mTimestampSource == "metadata") {
      const auto iterMetadata = moMetadata->getMetadataMap().find(mTimestampMetaField);
      const bool isFound = iterMetadata != moMetadata->getMetadataMap().end();
      if (isFound) {
        mTimestampAnchor = std::stoll(iterMetadata->second);
      } else {
        ILOG(Error) << "Cannot find timestamp metadata field " << mTimestampMetaField << " in hist " << mMetaAnchorInput << " . Setting timestamp to -1" << ENDM;
        mTimestampAnchor = -1;
      }
    } else if (mTimestampSource == "current") {
      mTimestampAnchor = -1;
    } else {
      ILOG(Error) << "Uknown timestamp source " << mTimestampSource << " . Setting timestamp to -1" << ENDM;
    }
    // Get BC pattern
    std::map<std::string, std::string> metadata;
    std::map<std::string, std::string> headers;
    const auto ptrGRPLHCIFData = mCcdbApi.retrieveFromTFileAny<o2::parameters::GRPLHCIFData>(mPathGrpLhcIf, metadata, mTimestampAnchor, &headers);
    if (ptrGRPLHCIFData) {
      mGRPLHCIFData = *ptrGRPLHCIFData;
    } else {
      mGRPLHCIFData = o2::parameters::GRPLHCIFData();
      ILOG(Error) << "Cannot get GRPLHCIFData, setting default(zero) object";
    }
  }
  bool IsNonEmptySample() const { return mIsNonEmptySample; }
  void update(Trigger trg, framework::ServiceRegistryRef serviceReg)
  {
    setTrigger(trg);
    getMetadata();
    mIsFirstIter = false;
  }

 private:
  bool mIsMetadataValid{ false };
  bool mIsNonEmptySample{ false };
  bool mIsFirstIter{ true };
};

} // namespace o2::quality_control_modules::fit

#endif // QC_MODULE_FIT_POSTPROCHELPER_H
