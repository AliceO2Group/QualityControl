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
///

#include "FT0/GenericCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TMath.h>
// #include <TLine.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::ft0
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

  mCheckMaxOverflowIntegralRatio = getCheckFromConfig("MaxOverflowIntegralRatio");

  mCheckMinMeanX = getCheckFromConfig("MinMeanX");
  mCheckMaxMeanX = getCheckFromConfig("MaxMeanX");
  mCheckMaxStddevX = getCheckFromConfig("MaxStddevX");

  mCheckMinMeanY = getCheckFromConfig("MinMeanY");
  mCheckMaxMeanY = getCheckFromConfig("MaxMeanY");
  mCheckMaxStddevY = getCheckFromConfig("MaxStddevY");

  mCheckMinGraphLastPoint = getCheckFromConfig("MinGraphLastPoint");
  mCheckMaxGraphLastPoint = getCheckFromConfig("MaxGraphLastPoint");

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

} // namespace o2::quality_control_modules::ft0
