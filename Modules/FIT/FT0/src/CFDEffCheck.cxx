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
/// \file   CFDEffCheck.cxx
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#include "FT0/CFDEffCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::ft0
{

void CFDEffCheck::configure()
{
  if (auto param = mCustomParameters.find("thresholdWarning"); param != mCustomParameters.end()) {
    mThreshWarning = stof(param->second);
    ILOG(Debug, Support) << "configure() : using thresholdWarning = " << mThreshWarning << ENDM;
  } else {
    mThreshWarning = 0.9;
    ILOG(Debug, Support) << "configure() : using default thresholdWarning = " << mThreshWarning << ENDM;
  }

  if (auto param = mCustomParameters.find("thresholdError"); param != mCustomParameters.end()) {
    mThreshError = stof(param->second);
    ILOG(Debug, Support) << "configure() : using thresholdError = " << mThreshError << ENDM;
  } else {
    mThreshError = 0.8;
    ILOG(Debug, Support) << "configure() : using default thresholdError = " << mThreshError << ENDM;
  }

  if (auto param = mCustomParameters.find("deadChannelMap"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    std::vector<uint8_t> deadChannelVec = parseParameters<uint8_t>(chIDs, del);

    mDeadChannelMap = new o2::fit::DeadChannelMap();
    for (uint8_t chId = 0; chId < sNCHANNELS; ++chId) {
      if (std::find(deadChannelVec.begin(), deadChannelVec.end(), chId) != deadChannelVec.end())
        mDeadChannelMap->setChannelAlive(chId, 0);
      else
        mDeadChannelMap->setChannelAlive(chId, 1);
    }
    ILOG(Warning, Support) << "configure() : using deadChannelMap from config (superseding the one from CCDB)" << ENDM;
  } else {
    if (auto param = mCustomParameters.find("ccdbUrl"); param != mCustomParameters.end()) {
      setCcdbUrl(param->second);
      ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, configured url = " << param->second << ENDM;
    } else {
      setCcdbUrl("o2-ccdb.internal");
      ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, default url = "
                           << "o2-ccdb.internal" << ENDM;
    }
    if (auto param = mCustomParameters.find("pathDeadChannelMap"); param != mCustomParameters.end()) {
      mPathDeadChannelMap = param->second;
      ILOG(Debug, Support) << "configure() : using pathDeadChannelMap: " << mPathDeadChannelMap << ENDM;
    } else {
      mPathDeadChannelMap = "FT0/Calib/DeadChannelMap";
      ILOG(Debug, Support) << "configure() : using default pathDeadChannelMap: " << mPathDeadChannelMap << ENDM;
    }

    // WARNING: always uses last available dead channel map
    //          supply deadChannelMap by hand when running offline
    mDeadChannelMap = retrieveConditionAny<o2::fit::DeadChannelMap>(mPathDeadChannelMap);
    if (!mDeadChannelMap || !mDeadChannelMap->map.size()) {
      ILOG(Error, Support) << "object \"" << mPathDeadChannelMap << "\" NOT retrieved (or empty). All channels assumed to be alive!" << ENDM;
      mDeadChannelMap = new o2::fit::DeadChannelMap();
      for (uint8_t chId = 0; chId < sNCHANNELS; ++chId) {
        mDeadChannelMap->setChannelAlive(chId, 1);
      }
    }
  }
  mDeadChannelMapStr = "";
  for (unsigned chId = 0; chId < mDeadChannelMap->map.size(); chId++) {
    if (!mDeadChannelMap->isChannelAlive(chId)) {
      mDeadChannelMapStr += (mDeadChannelMapStr.empty() ? "" : ",") + std::to_string(chId);
    }
  }
  if (mDeadChannelMapStr.empty())
    mDeadChannelMapStr = "EMPTY";
  ILOG(Info, Support) << "Loaded dead channel map: " << mDeadChannelMapStr << ENDM;
}

Quality CFDEffCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "CFD_efficiency") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F* => Quality::Bad" << ENDM;
        result = Quality::Bad;
        continue;
      }

      result = Quality::Good;
      mNumErrors = 0;
      mNumWarnings = 0;
      for (uint8_t chId = 0; chId < h->GetNbinsX(); chId++) {
        if (chId >= sNCHANNELS)
          continue;
        if (!mDeadChannelMap->isChannelAlive(chId))
          continue;
        if (h->GetBinContent(chId + 1) < mThreshError) {
          if (result.isBetterThan(Quality::Bad))
            // result = Quality::Bad; // setting quality like this clears reasons
            result.set(Quality::Bad);
          mNumErrors++;
          result.addReason(FlagReasonFactory::Unknown(),
                           "CFD eff. < \"Error\" threshold in channel " + std::to_string(chId));
          // no need to check medium threshold
          // but don't `break` because we want to add other reasons
          continue;
        } else if (h->GetBinContent(chId + 1) < mThreshWarning) {
          if (result.isBetterThan(Quality::Medium))
            result.set(Quality::Medium);
          mNumWarnings++;
          result.addReason(FlagReasonFactory::Unknown(),
                           "CFD eff. < \"Warning\" threshold in channel " + std::to_string(chId));
        }
      }
    }
  }
  result.addMetadata("nErrors", std::to_string(mNumErrors));
  result.addMetadata("nWarnings", std::to_string(mNumWarnings));
  return result;
}

std::string CFDEffCheck::getAcceptedType() { return "TH1"; }

void CFDEffCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "CFD_efficiency") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    TPaveText* msg = new TPaveText(0.15, 0.2, 0.85, 0.45, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    if (mDeadChannelMapStr != "EMPTY")
      msg->AddText(("Dead channel IDs: " + mDeadChannelMapStr).c_str());
    msg->AddText(Form("N channels with warning (< %.3f) = %d", mThreshWarning, mNumWarnings));
    msg->AddText(Form("N channels with error   (< %.3f) = %d", mThreshError, mNumErrors));

    if (checkResult == Quality::Good) {
      msg->AddText(">> Quality::Good <<");
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      auto reasons = checkResult.getReasons();
      msg->SetFillColor(kRed);
      msg->AddText(">> Quality::Bad <<");
    } else if (checkResult == Quality::Medium) {
      auto reasons = checkResult.getReasons();
      msg->SetFillColor(kOrange);
      msg->AddText(">> Quality::Medium <<");
    } else if (checkResult == Quality::Null) {
      msg->AddText(">> Quality::Null <<");
      msg->SetFillColor(kGray);
    }
    // add threshold lines
    Double_t xMin = h->GetXaxis()->GetXmin();
    Double_t xMax = h->GetXaxis()->GetXmax();
    auto* lineError = new TLine(xMin, mThreshError, xMax, mThreshError);
    auto* lineWarning = new TLine(xMin, mThreshWarning, xMax, mThreshWarning);
    lineError->SetLineWidth(3);
    lineWarning->SetLineWidth(3);
    lineError->SetLineStyle(kDashed);
    lineWarning->SetLineStyle(kDashed);
    lineError->SetLineColor(kRed);
    lineWarning->SetLineColor(kOrange);
    h->GetListOfFunctions()->Add(lineError);
    h->GetListOfFunctions()->Add(lineWarning);
    h->SetStats(0);
  }
}

} // namespace o2::quality_control_modules::ft0
