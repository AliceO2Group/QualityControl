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

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
#include "Common/Utils.h"
using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fit
{

void LevelCheck::updateBinsToIgnoreWithDCM()
{
  if (mPathDeadChannelMap.size() > 0) {
    const auto deadChannelMap = retrieveConditionAny<o2::fit::DeadChannelMap>(mPathDeadChannelMap, {}, mTimestamp);
    for (unsigned chID = 0; chID < deadChannelMap->map.size(); chID++) {
      if (!deadChannelMap->isChannelAlive(chID)) {
        mBinsToIgnore.insert(chID);
      }
    }
  }
}

void LevelCheck::startOfActivity(const Activity&)
{
  mIsFirstIter = true; // for SSS
}

void LevelCheck::configure()
{
  mIsFirstIter = true;
  mMessagePrefixWarning = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "messagePrefixWarning", "Warning in bin idxs: ");
  mMessagePrefixError = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "messagePrefixError", "Error in bin idxs: ");
  mTimestampMetaField = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "timestampMetaField", "timestampTF");
  mTimestampSource = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "timestampSource", "metadata");

  mThreshWarning = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "thresholdWarning", 0.9);
  mThreshError = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "thresholdError", 0.8);
  mNameObjectToCheck = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "nameObjectToCheck", "CFD_efficiency");

  mNelementsPerLine = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nElementsPerLine", 20);
  mUseBinLabels = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "useBinLabels", false);
  mUseBinError = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "useBinError", false);

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
  mBinsToIgnoreAsStr = std::to_string(mBinsToIgnore.size());
}

void LevelCheck::setTimestamp(const std::shared_ptr<MonitorObject>& moMetadata)
{
  // Getting timestamp
  if (mTimestampSource == "metadata") {
    const auto iterMetadata = moMetadata->getMetadataMap().find(mTimestampMetaField);
    const bool isFound = iterMetadata != moMetadata->getMetadataMap().end();
    if (isFound) {
      mTimestamp = std::stoll(iterMetadata->second);
    } else {
      ILOG(Error) << "Cannot find timestamp metadata field " << mTimestampMetaField << " . Setting timestamp to -1" << ENDM;
      mTimestamp = -1;
    }
  } else if (mTimestampSource == "current") {
    mTimestamp = -1;
  } else {
    mTimestamp = -1;
    ILOG(Error) << "Uknown timestamp source " << mTimestampSource << " . Setting timestamp to -1" << ENDM;
  }
}

Quality LevelCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  for (const auto& entry : *moMap) {
    const auto& mo = entry.second;
    if (mo->getName() == mNameObjectToCheck) {
      auto hist = dynamic_cast<TH1*>(mo->getObject());
      if (hist == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << mo->getName() << " to TH1* => Quality::Bad" << ENDM;
        result = Quality::Bad;
        result.addFlag(FlagTypeFactory::Unknown(), "Cannot get TH1 object from DB");
        continue;
      }
      if (mIsFirstIter) {
        setTimestamp(mo);
        // Automatic selection of bins to ignore
        // Dead channel map
        updateBinsToIgnoreWithDCM();
        mBinsToIgnoreAsStr = std::to_string(mBinsToIgnore.size());
        mIsFirstIter = false;
      }

      result = Quality::Good;
      std::vector<std::string> vecWarnings{};
      std::vector<std::string> vecErrors{};
      for (int binIdx = 0; binIdx < hist->GetNbinsX(); binIdx++) {
        if (mBinsToIgnore.find(binIdx) != mBinsToIgnore.end()) {
          continue;
        }
        const auto bin = binIdx + 1;
        const auto val = hist->GetBinContent(bin);
        const bool hasError = hist->GetBinError(bin) > 0; // for checking denomenator, if denominator == 0 error will be also zero
        const bool isError = (val < mThreshError && !mIsInvertedThrsh) || (val > mThreshError && mIsInvertedThrsh) || (!hasError && mUseBinError);
        const bool isWarning = (val < mThreshWarning && !mIsInvertedThrsh) || (val > mThreshWarning && mIsInvertedThrsh);
        const std::string binAsStr = mUseBinLabels ? hist->GetXaxis()->GetBinLabel(bin) : std::to_string(binIdx);
        if (isError) {
          vecErrors.push_back(binAsStr);
          continue;
        } else if (isWarning) {
          vecWarnings.push_back(binAsStr);
        }
      }
      mNumErrors = vecErrors.size();
      mNumWarnings = vecWarnings.size();
      auto funcInt2Str = [](std::string accum, std::string bin) { return std::move(accum) + ", " + bin; };
      auto addFlag = [&funcInt2Str, &result](auto&& vec, auto&& messagePrefix) -> void {
        if (vec.size() == 0) {
          return;
        }
        const auto idxLine = vec.size() > 0 ? std::accumulate(std::next(vec.begin()), vec.end(), vec[0], funcInt2Str) : "";
        const std::string message = messagePrefix + idxLine;
        result.addFlag(FlagTypeFactory::Unknown(), message);
      };
      auto addMultipleFlags = [&addFlag, this](auto&& vec, auto&& messagePrefix) -> void {
        if (vec.size() == 0) {
          return;
        }
        auto it = vec.begin();
        int distance{ 0 };
        do {
          const std::string msg = distance == 0 ? messagePrefix : ""; // first iteration, message prefix will be used only in first line
          const int extraNelements = distance == 0 ? 0 : mNelementsPerLine;
          const auto itBegin = it;
          std::advance(it, mNelementsPerLine + extraNelements);
          distance = std::distance(it, vec.end());
          const auto itEnd = distance > 0 ? it : vec.end();
          addFlag(std::vector<std::string>(itBegin, itEnd), msg);
        } while (distance > 0);
      };
      if (mNumErrors > 0) {
        if (result.isBetterThan(Quality::Bad)) {
          result.set(Quality::Bad);
        }
        addMultipleFlags(vecErrors, mMessagePrefixError);
      }
      if (mNumWarnings > 0) {
        if (result.isBetterThan(Quality::Medium)) {
          result.set(Quality::Medium);
        }
        addMultipleFlags(vecWarnings, mMessagePrefixWarning);
      }
    }
  }
  result.addMetadata("nErrors", std::to_string(mNumErrors));
  result.addMetadata("nWarnings", std::to_string(mNumWarnings));
  return result;
}

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
      auto flags = checkResult.getFlags();
      msg->SetFillColor(kRed);
      msg->AddText(">> Quality::Bad <<");
    } else if (checkResult == Quality::Medium) {
      auto flags = checkResult.getFlags();
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
