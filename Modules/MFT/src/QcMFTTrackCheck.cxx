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
/// \file   QcMFTTrackCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Diana Maria Krupova
/// \author Katarina Krizkova Gajdosova
// C++
#include <string>
// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH2.h>
#include <TPaveText.h>
// O2
#include "ITSMFTBase/DPLAlpideParam.h"
// Quality Control
#include "MFT/QcMFTTrackCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/UserCodeInterface.h"
#include "QualityControl/CustomParameters.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

void QcMFTTrackCheck::configure()
{
  // loading custom parameters
  mOnlineQC = 1;
  if (auto param = mCustomParameters.find("onlineQC"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - onlineQC: " << param->second << ENDM;
    mOnlineQC = stoi(param->second);
  }
  // no call to beautifier yet
  mFirstCall = true;
}

Quality QcMFTTrackCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;

    if (mo->getName() == "mClusterRatioVsBunchCrossing") {
      auto* hGroupedClusterSizeSummary = dynamic_cast<TH2F*>(mo->getObject());
      if (hGroupedClusterSizeSummary == nullptr) {
        ILOG(Error, Support) << "Could not cast mClusterRatioVsBunchCrossing to TH2F." << ENDM;
        return Quality::Null;
      }
    }
  }
  return result;
}

std::string QcMFTTrackCheck::getAcceptedType() { return "TH1"; }

void QcMFTTrackCheck::readAlpideCCDB(std::shared_ptr<MonitorObject> mo)
{
  mROF = 0;
  if (mOnlineQC == 1) {
    long timestamp = o2::ccdb::getCurrentTimestamp();
    map<string, string> headers;
    map<std::string, std::string> filter;
    auto alpideParam = UserCodeInterface::retrieveConditionAny<o2::itsmft::DPLAlpideParam<o2::detectors::DetID::MFT>>("MFT/Config/AlpideParam/", filter, timestamp);
    if (alpideParam == nullptr) {
      ILOG(Error, Support) << "Could not retrieve Alpide strobe length from CCDB." << ENDM;
      return;
    }
    mROF = alpideParam->roFrameLengthInBC;
  } else { // load the CCDB object in async processing
    o2::ccdb::CcdbApi api_ccdb;
    api_ccdb.init("alice-ccdb.cern.ch");
    int runNo;
    long timestamp = -1;

    map<string, string> hdRCT = api_ccdb.retrieveHeaders("RCT/Info/RunInformation", map<string, string>(), runNo);
    const auto startRCT = hdRCT.find("STF");
    if (startRCT != hdRCT.end()) {
      timestamp = stol(startRCT->second);
    } else {
      ILOG(Error, Support) << "STF not found in headers!" << ENDM;
    }
    map<string, string> headers;
    map<std::string, std::string> filter;
    auto alpideParam = UserCodeInterface::retrieveConditionAny<o2::itsmft::DPLAlpideParam<o2::detectors::DetID::MFT>>("MFT/Config/AlpideParam/", filter, timestamp);
    if (alpideParam == nullptr) {
      ILOG(Error, Support) << "Could not retrieve Alpide strobe length from CCDB." << ENDM;
      return;
    }
    mROF = alpideParam->roFrameLengthInBC;
  }
}

void QcMFTTrackCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mFirstCall) {
    mFirstCall = false;
    readAlpideCCDB(mo);
  }
  if (mo->getName().find("mClusterRatioVsBunchCrossing") != std::string::npos) {
    auto* hClusterRatioVsBunchCrossing = dynamic_cast<TH2F*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.73, 0.9, 0.95, 1.0, "NDC NB");
    hClusterRatioVsBunchCrossing->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->AddText(Form("ROF length from CCDB = %i", mROF));
    msg->Draw();
  }
}

} // namespace o2::quality_control_modules::mft
