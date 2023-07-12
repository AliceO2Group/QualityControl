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
/// \file   FractionCheck.cxx
/// \author Artur Furs afurs@cern.ch
///

#include "FT0/FractionCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>
#include "Common/Utils.h"
using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::ft0
{

void FractionCheck::configure()
{
  mThreshWarning = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "thresholdWarning", 0.9);
  mThreshError = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "thresholdError", 0.8);
  mNameObjectToCheck = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "nameObjectToCheck", "CFD_efficiency");
  mIsInversedThresholds = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "isInversedThresholds", false);
  const std::string binsToIgnore = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "binsToIgnore", "");
  if (binsToIgnore.size() > 0) {
    const auto vecParams = parseParameters<int>(binsToIgnore, ",");
    for (const auto& chId : vecParams) {
      mIgnoreBins.insert(chId);
    }
  }
  mUseDeadChannelMap = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "useDeadChannelMap", false);
  if (mUseDeadChannelMap) {
    const std::string ccdbUrl = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "ccdbUrl", "o2-ccdb.internal");
    setCcdbUrl(ccdbUrl);
    const std::string mPathDeadChannelMap = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "pathDeadChannelMap", "FT0/Calib/DeadChannelMap");
    mDeadChannelMap = retrieveConditionAny<o2::fit::DeadChannelMap>(mPathDeadChannelMap);
    for (unsigned chId = 0; chId < mDeadChannelMap->map.size(); chId++) {
      if (!mDeadChannelMap->isChannelAlive(chId)) {
        mIgnoreBins.insert(chId);
      }
    }
  }
  for (const auto chId : mIgnoreBins) {
    mDeadChannelMapStr += (mDeadChannelMapStr.empty() ? "" : ",") + std::to_string(chId);
  }
  if (mIgnoreBins.size() == 0) {
    mDeadChannelMapStr = "EMPTY";
  }
}

Quality FractionCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == mNameObjectToCheck) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1F* => Quality::Bad" << ENDM;
        result = Quality::Bad;
        continue;
      }

      result = Quality::Good;
      mNumErrors = 0;
      mNumWarnings = 0;
      for (int chId = 0; chId < h->GetNbinsX(); chId++) {
        if (chId >= sNCHANNELS) {
          continue;
        }
        if (mIgnoreBins.find(chId) != mIgnoreBins.end()) {
          continue;
        }
        const bool isError = (h->GetBinContent(chId + 1) < mThreshError && !mIsInversedThresholds) || (h->GetBinContent(chId + 1) > mThreshError && mIsInversedThresholds);
        const bool isWarning = ((h->GetBinContent(chId + 1) < mThreshWarning && !mIsInversedThresholds) || (h->GetBinContent(chId + 1) > mThreshWarning && mIsInversedThresholds));
        if (isError) {
          if (result.isBetterThan(Quality::Bad))
            // result = Quality::Bad; // setting quality like this clears reasons
            result.set(Quality::Bad);
          mNumErrors++;
          result.addReason(FlagReasonFactory::Unknown(),
                           "CFD eff. < \"Error\" threshold in channel " + std::to_string(chId));
          // no need to check medium threshold
          // but don't `break` because we want to add other reasons
          continue;
        } else if (isWarning) {
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

std::string FractionCheck::getAcceptedType() { return "TH1"; }

void FractionCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == mNameObjectToCheck) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    TPaveText* msg = new TPaveText(0.15, 0.2, 0.85, 0.45, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    if (mDeadChannelMapStr != "EMPTY")
      msg->AddText(("Ignore bins(ChannelIDs): " + mDeadChannelMapStr).c_str());
    if (!mIsInversedThresholds) {
      msg->AddText(Form("N channels with warning (< %.3f) = %d", mThreshWarning, mNumWarnings));
      msg->AddText(Form("N channels with error   (< %.3f) = %d", mThreshError, mNumErrors));
    } else {
      msg->AddText(Form("N channels with warning (> %.3f) = %d", mThreshWarning, mNumWarnings));
      msg->AddText(Form("N channels with error   (> %.3f) = %d", mThreshError, mNumErrors));
    }
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
