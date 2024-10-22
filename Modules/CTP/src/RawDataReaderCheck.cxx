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
/// \file   RawDataReaderCheck.cxx
/// \author Lucia Anna Tarasovicova
///

#include "CTP/RawDataReaderCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// #include "DataFormatsCTP/RunManager.h"
#include <DataFormatsCTP/Configuration.h>
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include <TMath.h>
#include <DataFormatsParameters/GRPLHCIFData.h>
#include <DataFormatsQualityControl/FlagType.h>
#include <DetectorsBase/GRPGeomHelper.h>
#include "Common/Utils.h"
#include <CommonUtils/StringUtils.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::ctp;

namespace o2::quality_control_modules::ctp
{

void RawDataReaderCheck::configure()
{
  // reading the parameters from the config.json
  // if not available, setting a default value
  // the threshold values are nSigma
  mThresholdRateBad = common::getFromConfig<float>(mCustomParameters, "thresholdRateBad", 3.);
  if (mThresholdRateBad > 4 || mThresholdRateBad < 0) {
    mThresholdRateBad = 3;
  }

  mThresholdRateMedium = common::getFromConfig<float>(mCustomParameters, "thresholdRateMedium", 2.);
  if (mThresholdRateMedium > 4 || mThresholdRateMedium < 0) {
    mThresholdRateMedium = 2;
  }

  mThresholdRateRatioBad = common::getFromConfig<float>(mCustomParameters, "thresholdRateRatioBad", 3.);
  if (mThresholdRateRatioBad > 4 || mThresholdRateRatioBad < 0) {
    mThresholdRateRatioBad = 3;
  }

  mThresholdRateRatioMedium = common::getFromConfig<float>(mCustomParameters, "thresholdRateRatioMedium", 2.);
  if (mThresholdRateRatioMedium > 4 || mThresholdRateRatioMedium < 0) {
    mThresholdRateRatioMedium = 2;
  }

  mNSigBC = common::getFromConfig<float>(mCustomParameters, "nSigmaBC", 2.);
  if (mNSigBC < 0) {
    mNSigBC = 2;
  }
}

Quality RawDataReaderCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  clearIndexVectors();
  mFlagRatio = false;
  mFlagInput = false;

  for (auto& [moName, mo] : *moMap) {
    if (moName.find("Ratio") != std::string::npos) {
      mFlagRatio = true;
    }
    if (moName.find("input") != std::string::npos) {
      mFlagInput = true;
    }
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (!h) {
      ILOG(Info, Support) << "histogram is not found for check:" << moName << ENDM;
      continue;
    }
    if (mo->getName() == "bcMinBias1" || mo->getName() == "bcMinBias2") {
      if (mLHCBCs.count() == 0) {
        continue;
      }
      float average = h->GetEntries() / mLHCBCs.count();
      mThreshold = average - mNSigBC * sqrt(average);
      if (mThreshold < std::sqrt(average)) {
        mThreshold = average / 2;
      }
      for (int i = 0; i < o2::constants::lhc::LHCMaxBunches; i++) {
        if (mLHCBCs[i] && h->GetBinContent(i + 1) <= mThreshold) {
          mVecMediumBC.push_back(i); // medium BC occures when BC is expected on this possition but there is less inputs than threshold
        } else if (!mLHCBCs[i] && h->GetBinContent(i + 1) > mThreshold) {
          mVecBadBC.push_back(i); // bad BC occures when BC is not expected on this possition but there is more inputs than threshold
        } else if (mLHCBCs[i] && h->GetBinContent(i + 1) > mThreshold) {
          mVecGoodBC.push_back(i);
        }
      }
      result.set(setQualityResult(mVecBadBC, mVecMediumBC));
    } else if (mo->getName() == "inputs") {
      if (mHistInputPrevious) {
        checkChange(h, mHistInputPrevious);
        delete mHistInputPrevious;
        result.set(setQualityResult(mVecIndexBad, mVecIndexMedium));
      }
      mHistAbsolute = (TH1D*)h->Clone();
      mHistInputPrevious = (TH1D*)h->Clone();
    } else if (mo->getName() == "inputRatio") {
      if (mHistInputRatioPrevious) {
        checkChangeOfRatio(h, mHistInputRatioPrevious, mHistAbsolute);
        delete mHistInputRatioPrevious;
        result.set(setQualityResult(mVecIndexBad, mVecIndexMedium));
      }
      mHistInputRatioPrevious = (TH1D*)h->Clone();
    } else if (mo->getName() == "classes") {
      if (mHistClassesPrevious) {
        checkChange(h, mHistClassesPrevious);
        delete mHistClassesPrevious;
        result.set(setQualityResult(mVecIndexBad, mVecIndexMedium));
      }
      mHistAbsolute = (TH1D*)h->Clone();
      mHistClassesPrevious = (TH1D*)h->Clone();
    } else if (mo->getName() == "classRatio") {
      if (mHistClassRatioPrevious) {
        checkChangeOfRatio(h, mHistClassRatioPrevious, mHistAbsolute);
        delete mHistClassRatioPrevious;
        result.set(setQualityResult(mVecIndexBad, mVecIndexMedium));
      }
      mHistClassRatioPrevious = (TH1D*)h->Clone();
    } else {
      ILOG(Info, Support) << "Unknown histo:" << moName << ENDM;
    }
  }

  return result;
}

int RawDataReaderCheck::checkChange(TH1D* mHist, TH1D* mHistPrev)
{
  /// this function check how much the rates differ from previous cycle
  float thrBad = mThresholdRateBad;
  float thrMedium = mThresholdRateMedium;
  for (size_t i = 1; i < mHist->GetXaxis()->GetNbins() + 1; i++) { // Check how many inputs/classes changed more than a threshold value
    double val = mHist->GetBinContent(i);
    double valPrev = mHistPrev->GetBinContent(i);
    double relDiff = (valPrev != 0 || val != 0) ? (val - valPrev) / TMath::Sqrt(val) : 0;
    if (TMath::Abs(relDiff) > thrBad) {
      mVecIndexBad.push_back(i - 1);
    } else if (TMath::Abs(relDiff) > thrMedium) {
      mVecIndexMedium.push_back(i - 1);
    }
  }
  return 0;
}
int RawDataReaderCheck::checkChangeOfRatio(TH1D* mHist, TH1D* mHistPrev, TH1D* mHistAbs)
{
  /// this function check how much the rates differ from previous cycle
  float thrBad = mThresholdRateRatioBad;
  float thrMedium = mThresholdRateRatioMedium;
  int binMB;
  for (int i = 1; i < mHist->GetXaxis()->GetNbins() + 1; i++) {
    if (mHist->GetBinContent(i) == mHistPrev->GetBinContent(i) && mHist->GetBinContent(i) == 1) {
      binMB = i;
      break;
    }
  }
  for (size_t i = 1; i < mHist->GetXaxis()->GetNbins() + 1; i++) { // Check how many inputs/classes changed more than a threshold value
    double val = mHist->GetBinContent(i);
    double valPrev = mHistPrev->GetBinContent(i);
    double relDiff = (val != 0 || mHistAbs->GetBinContent(i) != 0) ? (val - valPrev) / (val * TMath::Sqrt(1 / mHistAbs->GetBinContent(binMB) + 1 / mHistAbs->GetBinContent(i))) : 0;
    if (TMath::Abs(relDiff) > thrBad) {
      mVecIndexBad.push_back(i - 1);
    } else if (TMath::Abs(relDiff) > thrMedium) {
      mVecIndexMedium.push_back(i - 1);
    }
  }
  return 0;
}

std::string RawDataReaderCheck::getAcceptedType() { return "TH1"; }

void RawDataReaderCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::shared_ptr<TLatex> msg;
  if (mo->getName() == "bcMinBias1" || mo->getName() == "bcMinBias2") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    h->GetXaxis()->SetTitle("BC");
    if (checkResult != Quality::Null) {
      msg = std::make_shared<TLatex>(0.2, 0.85, Form("Quality: %s", (checkResult.getName()).c_str()));
      if (checkResult == Quality::Bad) {
        msg->SetTextColor(kRed);
      } else if (checkResult == Quality::Medium) {
        msg->SetTextColor(kOrange);
      } else if (checkResult == Quality::Good) {
        msg->SetTextColor(kGreen + 1);
      }
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
    }

    if (checkResult == Quality::Null) {
      msg = std::make_shared<TLatex>(0.2, 0.8, Form("Check was not performed, LHC information not available"));
      msg->SetTextColor(kBlack);
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
    } else {
      msg = std::make_shared<TLatex>(0.2, 0.8, Form("Number of good BC: %lu", mVecGoodBC.size()));
      msg->SetTextColor(kBlack);
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());

      if (mVecMediumBC.size() > 0) {
        msg = std::make_shared<TLatex>(0.2, 0.75, Form("BC is expected on following possitions, but inputs are below threshold (%f):", mThreshold));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
        for (size_t i = 0; i < mVecMediumBC.size(); i++) {
          msg = std::make_shared<TLatex>(0.2, 0.75 - (i + 1) * 0.02, Form("%d", mVecMediumBC[i]));
          msg->SetTextSize(0.02);
          msg->SetNDC();
          h->GetListOfFunctions()->Add(msg->Clone());
        }
      }

      if (mVecBadBC.size() > 0) {
        msg = std::make_shared<TLatex>(0.2, 0.75, Form("BC is not expected on following possitions, but inputs are above threshold (%f):", mThreshold));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
        for (size_t i = 0; i < mVecBadBC.size(); i++) {
          msg = std::make_shared<TLatex>(0.2, 0.75 - (i + 1) * 0.02, Form("%d", mVecBadBC[i]));
          msg->SetTextSize(0.02);
          msg->SetNDC();
          h->GetListOfFunctions()->Add(msg->Clone());
        }
      }
    }
    h->SetStats(kFALSE);
    h->GetYaxis()->SetRangeUser(0, h->GetMaximum() * 1.5);
  } else {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    h->SetStats(kFALSE);
    msg = std::make_shared<TLatex>(0.45, 0.8, Form("Quality: %s", (checkResult.getName()).c_str()));
    std::string groupName = "Input";
    std::string relativeness = "relative";
    if (!mFlagInput) {
      groupName = "Class";
    }
    if (!mFlagRatio) {
      relativeness = "absolute";
    }
    if (mFlagInput) {
      for (size_t i = 0; i < h->GetXaxis()->GetNbins(); i++) {
        std::string label = o2::ctp::CTPInputsConfiguration::getInputNameFromIndex(i + 1);
        if (label == "none") {
          h->GetXaxis()->SetBinLabel(i + 1, Form("%zu", i + 1));
        } else {
          h->GetXaxis()->SetBinLabel(i + 1, label.c_str());
        }
      }
      h->GetXaxis()->SetLabelSize(0.045);
      h->GetXaxis()->LabelsOption("v");
      h->GetXaxis()->CenterLabels(true);
    }
    if (checkResult == Quality::Bad) {
      msg->SetTextColor(kRed);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      msg = std::make_shared<TLatex>(0.45, 0.70, Form("Number of %s with big %s rate change: %lu", groupName.c_str(), relativeness.c_str(), mVecIndexBad.size()));
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      for (size_t i = 0; i < mVecIndexBad.size(); i++) {
        if (mFlagInput) {
          msg = std::make_shared<TLatex>(0.45, 0.65 - i * 0.05, Form("Check %s %s", o2::ctp::CTPInputsConfiguration::getInputNameFromIndex(mVecIndexBad[i] + 1).c_str(), groupName.c_str()));
        } else {
          msg = std::make_shared<TLatex>(0.45, 0.65 - i * 0.05, Form("Check %s %s", groupName.c_str(), h->GetXaxis()->GetBinLabel(mVecIndexBad[i] + 1)));
        }
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
      if (mVecIndexMedium.size() > 0) {
        msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with medium %s rate change: %lu", groupName.c_str(), relativeness.c_str(), mVecIndexMedium.size()));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
    } else if (checkResult == Quality::Medium) {
      msg->SetTextColor(kOrange);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      if (mVecIndexMedium.size() > 0) {
        msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with medium %s rate change: %lu", groupName.c_str(), relativeness.c_str(), mVecIndexMedium.size()));
      }
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      for (size_t i = 0; i < mVecIndexMedium.size(); i++) {
        msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("%s with Index: %d", groupName.c_str(), mVecIndexMedium[i]));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
    } else if (checkResult == Quality::Good) {
      msg->SetTextColor(kGreen + 1);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
    }
    h->SetMarkerStyle(20);
  }
}

Quality RawDataReaderCheck::setQualityResult(std::vector<int>& vBad, std::vector<int>& vMedium)
{
  // setting a quality for a given hisogram based on nuber of bad/medium indices
  Quality quality = Quality::Null;
  if (vBad.size() > 0) {
    quality = Quality::Bad;
  } else if (vMedium.size() > 0) {
    quality = Quality::Medium;
  } else {
    quality = Quality::Good;
  }

  return quality;
}

void RawDataReaderCheck::clearIndexVectors()
{
  // clearing all vectors with saved indices
  mVecBadBC.clear();
  mVecMediumBC.clear();
  mVecGoodBC.clear();
  mVecIndexBad.clear();
  mVecIndexMedium.clear();
}

void RawDataReaderCheck::startOfActivity(const core::Activity& activity)
{
  mTimestamp = activity.mValidity.getMin();

  map<string, string> metadata; // can be empty
  auto lhcifdata = UserCodeInterface::retrieveConditionAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF", metadata, mTimestamp);
  if (lhcifdata == nullptr) {
    ILOG(Info, Support) << "LHC data not found for timestamp:" << mTimestamp << ENDM;
    return;
  } else {
    ILOG(Info, Support) << "LHC data found for timestamp:" << mTimestamp << ENDM;
  }
  auto bfilling = lhcifdata->getBunchFilling();
  std::vector<int> bcs = bfilling.getFilledBCs();
  mLHCBCs.reset();
  for (auto const& bc : bcs) {
    mLHCBCs.set(bc, 1);
  }
}

} // namespace o2::quality_control_modules::ctp
