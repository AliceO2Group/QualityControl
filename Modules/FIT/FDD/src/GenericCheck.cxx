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
/// \file   GenericCheck.cxx
/// \author Sebastian Bysiak
/// LATEST modification for FDD on 25.04.2023 (akhuntia@cern.ch)

#include "FDD/GenericCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TMath.h>
#include <TLine.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fdd
{

SingleCheck GenericCheck::getCheckFromConfig(std::string paramName)
{
  bool isActive = false;
  float thresholdWarning = TMath::SignalingNaN(), thresholdError = TMath::SignalingNaN();
  bool shouldBeLower = true;
  if (paramName.find("Max") != string::npos || paramName.find("max") != string::npos) {
    shouldBeLower = true;
  } else if (paramName.find("Min") != string::npos || paramName.find("min") != string::npos) {
    shouldBeLower = false;
  }

  if (auto paramWarning = mCustomParameters.find(string("thresholdWarning") + paramName), paramError = mCustomParameters.find(string("thresholdError") + paramName); paramWarning != mCustomParameters.end() || paramError != mCustomParameters.end()) {
    if (paramWarning != mCustomParameters.end() && paramError != mCustomParameters.end()) {
      thresholdWarning = stof(paramWarning->second);
      thresholdError = stof(paramError->second);
      isActive = true;
      if ((shouldBeLower && thresholdWarning > thresholdError) || (!shouldBeLower && thresholdWarning < thresholdError)) {
        ILOG(Warning, Support) << "configure(): warning more strict than error -> swapping values!" << ENDM;
        std::swap(thresholdWarning, thresholdError);
      }
      ILOG(Debug, Support) << "configure() : using thresholdWarning" << paramName.c_str() << " = " << thresholdWarning << " , thresholdError" << paramName << " = " << thresholdError << ENDM;
    } else {
      ILOG(Warning, Support) << "configure() : only one threshold (warning/error) was provided for  " << paramName.c_str() << " -> this parameter will not be used!" << ENDM;
    }
  }
  return SingleCheck(paramName, thresholdWarning, thresholdError, shouldBeLower, isActive);
}

void GenericCheck::configure()
{
  mCheckMaxThresholdY = getCheckFromConfig("MaxThresholdY");
  mCheckMinThresholdY = getCheckFromConfig("MinThresholdY");

  mCheckMaxOverflowIntegralRatio = getCheckFromConfig("MaxOverflowIntegralRatio");
  mCheckMinMeanX = getCheckFromConfig("MinMeanX");
  mCheckMaxMeanX = getCheckFromConfig("MaxMeanX");
  mCheckMaxStddevX = getCheckFromConfig("MaxStddevX");

  mCheckMinMeanY = getCheckFromConfig("MinMeanY");
  mCheckMaxMeanY = getCheckFromConfig("MaxMeanY");
  mCheckMaxStddevY = getCheckFromConfig("MaxStddevY");

  mCheckMinGraphLastPoint = getCheckFromConfig("MinGraphLastPoint");
  mCheckMaxGraphLastPoint = getCheckFromConfig("MaxGraphLastPoint");

  // Set path to ccdb to get DeadChannelMap
  if (auto param = mCustomParameters.find("ccdbUrl"); param != mCustomParameters.end()) {
    setCcdbUrl(param->second);
    ILOG(Info, Support) << "configure() : using deadChannelMap from CCDB, configured url = " << param->second << ENDM;
  } else {
    setCcdbUrl("alice-ccdb.cern.ch");
    ILOG(Debug, Support) << "configure() : using deadChannelMap from CCDB, default url = "
                         << "alice-ccdb.cern.ch" << ENDM;
  }

  // Set internal path to DeadChannelMap
  if (auto param = mCustomParameters.find("pathDeadChannelMap"); param != mCustomParameters.end()) {
    mPathDeadChannelMap = param->second;
    ILOG(Debug, Support) << "configure() : using pathDeadChannelMap: " << mPathDeadChannelMap << ENDM;
  } else {
    mPathDeadChannelMap = "FDD/Calib/DeadChannelMap";
    ILOG(Debug, Support) << "configure() : using default pathDeadChannelMap: " << mPathDeadChannelMap << ENDM;
  }

  // Align mDeadChannelMap with downloaded one
  mDeadChannelMap = retrieveConditionAny<o2::fit::DeadChannelMap>(mPathDeadChannelMap);
  if (!mDeadChannelMap || !mDeadChannelMap->map.size()) {
    ILOG(Error, Support) << "object \"" << mPathDeadChannelMap << "\" NOT retrieved (or empty). All channels assumed to be alive!" << ENDM;
    mDeadChannelMap = new o2::fit::DeadChannelMap();
    for (uint8_t chId = 0; chId < sNCHANNELSPhy; ++chId) {
      mDeadChannelMap->setChannelAlive(chId, 1);
    }
  }

  // Print DeadChannelMap
  mDeadChannelMapStr = "";
  for (unsigned chId = 0; chId < mDeadChannelMap->map.size(); chId++) {
    if (!mDeadChannelMap->isChannelAlive(chId)) {
      mDeadChannelMapStr += (mDeadChannelMapStr.empty() ? "" : ",") + std::to_string(chId);
    }
  }
  if (mDeadChannelMapStr.empty()) {
    mDeadChannelMapStr = "EMPTY";
  }
  ILOG(Info, Support) << "Loaded dead channel map: " << mDeadChannelMapStr << ENDM;

  mPositionMsgBox = { 0.15, 0.75, 0.85, 0.9 };
  if (auto param = mCustomParameters.find("positionMsgBox"); param != mCustomParameters.end()) {
    stringstream ss(param->second);
    int i = 0;
    while (ss.good()) {
      if (i > 4) {
        ILOG(Info, Support) << "Skipping next values provided for positionMsgBox" << ENDM;
        break;
      }
      string substr;
      getline(ss, substr, ',');
      mPositionMsgBox[i] = std::stod(substr);
      i++;
    }
    float minHeight = 0.09, minWidth = 0.19;
    if (mPositionMsgBox[2] - mPositionMsgBox[0] < minWidth || mPositionMsgBox[3] - mPositionMsgBox[1] < minHeight) {
      mPositionMsgBox = { 0.15, 0.75, 0.85, 0.9 };
      ILOG(Info, Support) << "MsgBox too small: returning to default" << ENDM;
    }
  }

  if (auto param = mCustomParameters.find("nameObjOnCanvas"); param != mCustomParameters.end()) {
    mNameObjOnCanvas = param->second;
    ILOG(Info, Support) << "nameObjOnCanvas set to " << mNameObjOnCanvas << ENDM;
  } else {
    mNameObjOnCanvas = "unspecified";
  }
}

Quality GenericCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Good;
  for (auto& [moName, mo] : *moMap) {
    if (!mo) {
      result.set(Quality::Null);
      ILOG(Error, Support) << "MO " << moName << " not found" << ENDM;
      continue;
    }
    if (std::strcmp(mo->getObject()->ClassName(), "TCanvas") == 0) {
      auto g = (TGraph*)((TCanvas*)mo->getObject())->GetListOfPrimitives()->FindObject(mNameObjOnCanvas.c_str());

      if (!g) {
        result.set(Quality::Null);
        ILOG(Error, Support) << "Object " << mNameObjOnCanvas << " inside MO " << moName << " not found" << ENDM;
        continue;
      }

      if (mCheckMinGraphLastPoint.isActive())
        mCheckMinGraphLastPoint.doCheck(result, g->GetPointY(g->GetN() - 1));
      if (mCheckMaxGraphLastPoint.isActive())
        mCheckMaxGraphLastPoint.doCheck(result, g->GetPointY(g->GetN() - 1));
    }
    if (std::strcmp(mo->getObject()->ClassName(), "TGraph") == 0) {
      auto g = dynamic_cast<TGraph*>(mo->getObject());

      if (!g) {
        result.set(Quality::Null);
        ILOG(Error, Support) << "Object inside MO " << moName << " not found" << ENDM;
        continue;
      }

      if (mCheckMinGraphLastPoint.isActive())
        mCheckMinGraphLastPoint.doCheck(result, g->GetPointY(g->GetN() - 1));
      if (mCheckMaxGraphLastPoint.isActive())
        mCheckMaxGraphLastPoint.doCheck(result, g->GetPointY(g->GetN() - 1));

    } else {
      auto h = dynamic_cast<TH1*>(mo->getObject());
      if (!h) {
        result.set(Quality::Null);
        ILOG(Error, Support) << "Object inside MO " << moName << " not found" << ENDM;
        continue;
      }

      if (mCheckMinThresholdY.isActive()) {
        float minValue = h->GetBinContent(1);
        for (int channel = 1; channel < h->GetNbinsX(); ++channel) {
          if (channel >= sNCHANNELSPhy || !mDeadChannelMap->isChannelAlive(channel)) {
            continue;
          }
          if (minValue > h->GetBinContent(channel)) {
            minValue = h->GetBinContent(channel);
            mCheckMinThresholdY.mBinNumberX = channel;
          }
        }
        mCheckMinThresholdY.doCheck(result, minValue);
      }

      if (mCheckMaxThresholdY.isActive()) {
        if (mDeadChannelMap->isChannelAlive(h->GetMaximumBin())) {
          mCheckMaxThresholdY.mBinNumberX = h->GetMaximumBin();
          mCheckMaxThresholdY.doCheck(result, h->GetBinContent(mCheckMaxThresholdY.mBinNumberX));
        } else {
          float maxValue = 0;
          for (int channel = 1; channel < h->GetNbinsX(); ++channel) {
            if (channel >= sNCHANNELSPhy || !mDeadChannelMap->isChannelAlive(channel)) {
              continue;
            }
            if (maxValue < h->GetBinContent(channel)) {
              maxValue = h->GetBinContent(channel);
              mCheckMaxThresholdY.mBinNumberX = channel;
            }
          }
          mCheckMaxThresholdY.doCheck(result, maxValue);
        }
      }

      if (mCheckMinMeanX.isActive())
        mCheckMinMeanX.doCheck(result, h->GetMean());
      if (mCheckMaxMeanX.isActive())
        mCheckMaxMeanX.doCheck(result, h->GetMean());
      if (mCheckMaxStddevX.isActive())
        mCheckMaxStddevX.doCheck(result, h->GetStdDev());

      if (mCheckMinMeanY.isActive())
        mCheckMinMeanY.doCheck(result, h->GetMean(2));
      if (mCheckMaxMeanY.isActive())
        mCheckMaxMeanY.doCheck(result, h->GetMean(2));
      if (mCheckMaxStddevY.isActive())
        mCheckMaxStddevY.doCheck(result, h->GetStdDev(2));

      if (mCheckMaxOverflowIntegralRatio.isActive()) {
        float integralWithoutOverflow = 0., overflow = 0.;
        if (h->GetDimension() == 1) {
          integralWithoutOverflow = h->Integral();
          overflow = h->GetBinContent(h->GetNbinsX() + 1);
        } else if (h->GetDimension() == 2) {
          // for 2D include these overflows: (over,over), (over,in-range), (in-range, over)
          TH2* h2 = (TH2*)h->Clone(); // TH2 needed for Integral() with both axis specified
          integralWithoutOverflow = h2->Integral();
          float integralWithOverflow = h2->Integral(1, h2->GetNbinsX() + 1, 1, h2->GetNbinsY() + 1);
          overflow = integralWithOverflow - integralWithoutOverflow;
        }
        mCheckMaxOverflowIntegralRatio.doCheck(result, overflow / integralWithoutOverflow);
      }
    }
  }
  return result;
}

std::string GenericCheck::getAcceptedType() { return "TObject"; }

void GenericCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (!mo) {
    ILOG(Error, Support) << "beautify(): MO not found" << ENDM;
    return;
  }

  TPaveText* msg = new TPaveText(mPositionMsgBox[0], mPositionMsgBox[1], mPositionMsgBox[2], mPositionMsgBox[3], "NDC");
  if (std::strcmp(mo->getObject()->ClassName(), "TCanvas") == 0) {
    auto g = (TGraph*)((TCanvas*)mo->getObject())->GetListOfPrimitives()->FindObject(mNameObjOnCanvas.c_str());
    if (!g) {
      ILOG(Error, Support) << "beautify(): Object " << mNameObjOnCanvas << " inside MO " << mo->GetName() << " not found" << ENDM;
      return;
    }
    g->GetListOfFunctions()->Add(msg);
  } else if (std::strcmp(mo->getObject()->ClassName(), "TGraph") == 0) {
    auto g = dynamic_cast<TGraph*>(mo->getObject());
    if (!g) {
      ILOG(Error, Support) << "beautify(): Object inside MO " << mo->GetName() << " not found" << ENDM;
      return;
    }
    g->GetListOfFunctions()->Add(msg);
  } else {
    auto h = dynamic_cast<TH1*>(mo->getObject());
    if (!h) {
      ILOG(Error, Support) << "beautify(): Object inside MO " << mo->GetName() << " not found" << ENDM;
      return;
    }
    h->GetListOfFunctions()->Add(msg);
    // add threshold lines
    if (mCheckMinThresholdY.isActive()) {
      Double_t xMin = h->GetXaxis()->GetXmin();
      Double_t xMax = h->GetXaxis()->GetXmax();
      auto* lineMinError = new TLine(xMin, mCheckMinThresholdY.getThresholdError(), xMax, mCheckMinThresholdY.getThresholdError());
      auto* lineMinWarning = new TLine(xMin, mCheckMinThresholdY.getThresholdWarning(), xMax, mCheckMinThresholdY.getThresholdWarning());
      lineMinError->SetLineWidth(3);
      lineMinWarning->SetLineWidth(3);
      lineMinError->SetLineStyle(kDashed);
      lineMinWarning->SetLineStyle(kDashed);
      lineMinError->SetLineColor(kRed);
      lineMinWarning->SetLineColor(kOrange);
      h->GetListOfFunctions()->Add(lineMinError);
      h->GetListOfFunctions()->Add(lineMinWarning);
    }
  }

  msg->SetName(Form("%s_msg", mo->GetName()));
  msg->Clear();
  auto reasons = checkResult.getReasons();
  for (int i = 0; i < int(reasons.size()); i++) {
    msg->AddText(reasons[i].second.c_str());
    if (i > 4) {
      msg->AddText("et al ... ");
      break;
    }
  }
  int color = kBlack;
  if (checkResult == Quality::Good) {
    color = kGreen + 1;
    msg->AddText(">> Quality::Good <<");
  } else if (checkResult == Quality::Medium) {
    color = kOrange - 1;
    msg->AddText(">> Quality::Medium <<");
  } else if (checkResult == Quality::Bad) {
    color = kRed;
    msg->AddText(">> Quality::Bad <<");
  }
  msg->SetFillStyle(1);
  msg->SetLineWidth(3);
  msg->SetLineColor(color);
  msg->SetShadowColor(color);
  msg->SetTextColor(color);
  msg->SetMargin(0.0);
}

} // namespace o2::quality_control_modules::fdd
