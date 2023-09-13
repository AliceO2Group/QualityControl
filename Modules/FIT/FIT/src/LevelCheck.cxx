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
/// \file   LevelCheck.cxx
/// \author Artur Furs afurs@cern.ch
///

#include <FIT/LevelCheck.h>
#include <FITCommon/HelperCommon.h>
#include "DataFormatsFIT/DeadChannelMap.h"
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

namespace o2::quality_control_modules::fit
{

void LevelCheck::updateBinsToIgnoreWithDCM()
{
  if (mPathDeadChannelMap.size() > 0) {
    const auto deadChannelMap = retrieveConditionAny<o2::fit::DeadChannelMap>(mPathDeadChannelMap);
    for (unsigned chID = 0; chID < deadChannelMap->map.size(); chID++) {
      if (!deadChannelMap->isChannelAlive(chID)) {
        mBinsToIgnore.insert(chID);
      }
    }
  }
}

void LevelCheck::configure()
{
  mThreshWarning = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "thresholdWarning", 0.9);
  mThreshError = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "thresholdError", 0.8);
  mNameObjectToCheck = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "nameObjectToCheck", "CFD_efficiency");

  mIsInvertedThrsh = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "isInversedThresholds", false);
  mSignCheck = mIsInvertedThrsh ? std::string{ ">" } : std::string{ "<" };

  mPathDeadChannelMap = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "pathDeadChannelMap", "");
  mUrlCCDB = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "ccdbUrl", "o2-ccdb.internal");
  setCcdbUrl(mUrlCCDB);

  const std::string labelPos = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "labelPos", "0.15, 0.2, 0.85, 0.45");
  mVecLabelPos = helper::parseParameters<double>(labelPos, ",");
  if (mVecLabelPos.size() != 4) {
    ILOG(Error, Devel) << "Incorrect label coordinates! Setting default." << ENDM;
    mVecLabelPos = { 0.15, 0.2, 0.85, 0.45 };
  }

  const std::string binsToIgnore = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "binsToIgnore", "");
  // Getting bins to ignore directly from config,
  // they will be combined with automatic bin selection
  if (binsToIgnore.size() > 0) {
    const auto vecParams = helper::parseParameters<int>(binsToIgnore, ",");
    for (const auto& chId : vecParams) {
      mBinsToIgnore.insert(chId);
    }
  }
  // Automatic selection of bins to ignore
  // Dead channel map
  updateBinsToIgnoreWithDCM();
  mBinsToIgnoreAsStr = std::to_string(mBinsToIgnore.size());
}

Quality LevelCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (const auto& entry : *moMap) {
    const auto& mo = entry.second;
    if (mo->getName() == mNameObjectToCheck) {
      auto* h = dynamic_cast<TH1*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1* => Quality::Bad" << ENDM;
        result = Quality::Bad;
        continue;
      }

      result = Quality::Good;
      mNumErrors = 0;
      mNumWarnings = 0;
      for (int bin = 0; bin < h->GetNbinsX(); bin++) {
        if (mBinsToIgnore.find(bin) != mBinsToIgnore.end()) {
          continue;
        }
        const auto val = h->GetBinContent(bin + 1);
        const bool isError = (val < mThreshError && !mIsInvertedThrsh) || (val > mThreshError && mIsInvertedThrsh);
        const bool isWarning = (val < mThreshWarning && !mIsInvertedThrsh) || (val > mThreshWarning && mIsInvertedThrsh);
        if (isError) {
          if (result.isBetterThan(Quality::Bad)) {
            result.set(Quality::Bad);
          }
          mNumErrors++;
          const std::string message = "Ratio " + mSignCheck + "\"Error\" threshold in element " + std::to_string(bin);
          result.addReason(FlagReasonFactory::Unknown(), message);
          continue;
        } else if (isWarning) {
          if (result.isBetterThan(Quality::Medium))
            result.set(Quality::Medium);
          mNumWarnings++;
          const std::string message = "Ratio " + mSignCheck + "\"Warning\" threshold in element " + std::to_string(bin);
          result.addReason(FlagReasonFactory::Unknown(), message);
        }
      }
    }
  }
  result.addMetadata("nErrors", std::to_string(mNumErrors));
  result.addMetadata("nWarnings", std::to_string(mNumWarnings));
  return result;
}

std::string LevelCheck::getAcceptedType() { return "TH1"; }

void LevelCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == mNameObjectToCheck) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    TPaveText* msg = new TPaveText(mVecLabelPos[0], mVecLabelPos[1], mVecLabelPos[2], mVecLabelPos[3], "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    msg->AddText(Form("N ignored elements: %s", mBinsToIgnoreAsStr.c_str()));
    msg->AddText(Form("N elements with warning (%s %.3f) = %d", mSignCheck.c_str(), mThreshWarning, mNumWarnings));
    msg->AddText(Form("N elements with error   (%s %.3f) = %d", mSignCheck.c_str(), mThreshError, mNumErrors));
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

} // namespace o2::quality_control_modules::fit
