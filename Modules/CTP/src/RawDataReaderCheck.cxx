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
#include "DataFormatsCTP/RunManager.h"
#include <DataFormatsCTP/Configuration.h>
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include <DataFormatsParameters/GRPLHCIFData.h>
#include <DataFormatsQualityControl/FlagType.h>
#include <DetectorsBase/GRPGeomHelper.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::ctp;

namespace o2::quality_control_modules::ctp
{

void RawDataReaderCheck::configure()
{
  try {
    mThresholdRateBad = std::stof(mCustomParameters["thresholdRateBad"]);
  } catch (const std::exception& e) {
    mThresholdRateBad = 0.15;
  }
  if (mThresholdRateBad > 1 || mThresholdRateBad < 0) {
    mThresholdRateBad = 0.15;
  }

  try {
    mThresholdRateMedium = std::stof(mCustomParameters["thresholdRateMedium"]);
  } catch (const std::exception& e) {
    mThresholdRateMedium = 0.1;
  }
  if (mThresholdRateMedium > 1 || mThresholdRateMedium < 0) {
    mThresholdRateMedium = 0.1;
  }

  try {
    mThresholdRateRatioBad = std::stof(mCustomParameters["thresholdRateRatioBad"]);
  } catch (const std::exception& e) {
    mThresholdRateRatioBad = 0.1;
  }
  if (mThresholdRateRatioBad > 1 || mThresholdRateRatioBad < 0) {
    mThresholdRateRatioBad = 0.1;
  }

  try {
    mThresholdRateRatioMedium = std::stof(mCustomParameters["thresholdRateRatioMedium"]);
  } catch (const std::exception& e) {
    mThresholdRateRatioMedium = 0.05;
  }
  if (mThresholdRateRatioMedium > 1 || mThresholdRateRatioMedium < 0) {
    mThresholdRateRatioMedium = 0.1;
  }

  try {
    mCycleDuration = std::stof(mCustomParameters["cycleDurationSeconds"]);
  } catch (const std::exception& e) {
    mCycleDuration = 60;
  }

  try {
    mFraction = std::stof(mCustomParameters["fraction"]);
  } catch (const std::exception& e) {
    mFraction = 0.1;
  }
}

Quality RawDataReaderCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  ClearIndexVectors();
  flagRatio = false;
  flagInput = false;

  for (auto& [moName, mo] : *moMap) {
    if (moName.find("Ratio") != std::string::npos) {
      flagRatio = true;
    }
    if (moName.find("input") != std::string::npos) {
      flagInput = true;
    }
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (!h) {
      ILOG(Info, Support) << "histogram is not found for check:" << moName << ENDM;
      continue;
    }
    if (mo->getName() == "bcMTVX") {
      if (mLHCBCs.count() == 0)
        continue;
      mThreshold = h->GetEntries() / mLHCBCs.count();
      mThreshold = sqrt(mThreshold);
      for (int i = 0; i < o2::constants::lhc::LHCMaxBunches; i++) {
        if (mLHCBCs[i] && h->GetBinContent(i + 1) <= mThreshold) {
          vMediumBC.push_back(i);
        } else if (!mLHCBCs[i] && h->GetBinContent(i + 1) > mThreshold) {
          vBadBC.push_back(i);
        } else if (mLHCBCs[i] && h->GetBinContent(i + 1) > mThreshold) {
          vGoodBC.push_back(i);
        }
      }
      result.set(SetQualityResult(vBadBC, vMediumBC));
    } else if (mo->getName() == "inputs") {
      if (fHistInputPrevious) {
        checkChange(h, fHistInputPrevious, vIndexBad, vIndexMedium);
        delete fHistInputPrevious;
        result.set(SetQualityResult(vIndexBad, vIndexMedium));
      }
      fHistInputPrevious = (TH1F*)h->Clone();
    } else if (mo->getName() == "inputRatio") {
      if (fHistInputRatioPrevious) {
        checkChange(h, fHistInputRatioPrevious, vIndexBad, vIndexMedium);
        delete fHistInputRatioPrevious;
        result.set(SetQualityResult(vIndexBad, vIndexMedium));
      }
      fHistInputRatioPrevious = (TH1F*)h->Clone();
    } else if (mo->getName() == "classes") {
      if (fHistClassesPrevious) {
        checkChange(h, fHistClassesPrevious, vIndexBad, vIndexMedium);
        delete fHistClassesPrevious;
        result.set(SetQualityResult(vIndexBad, vIndexMedium));
      }
      fHistClassesPrevious = (TH1F*)h->Clone();
    } else if (mo->getName() == "classRatio") {
      if (fHistClassRatioPrevious) {
        checkChange(h, fHistClassRatioPrevious, vIndexBad, vIndexMedium);
        delete fHistClassRatioPrevious;
        result.set(SetQualityResult(vIndexBad, vIndexMedium));
      }
      fHistClassRatioPrevious = (TH1F*)h->Clone();
    } else {
      ILOG(Info, Support) << "Unknown histo:" << moName << ENDM;
    }
  }

  return result;
}
int RawDataReaderCheck::checkChange(TH1F* fHist, TH1F* fHistPrev, std::vector<int>& vIndexBad, std::vector<int>& vIndexMedium)
{
  float thrBad = mThresholdRateBad;
  float thrMedium = mThresholdRateMedium;
  if (flagRatio) {
    thrBad = mThresholdRateRatioBad;
    thrMedium = mThresholdRateRatioMedium;
  }
  for (size_t i = 1; i < fHist->GetXaxis()->GetNbins() + 1; i++) { // Check how many inputs/classes changed more than a threshold value
    double val = fHist->GetBinContent(i);
    double valPrev = fHistPrev->GetBinContent(i);
    double relDiff = (valPrev != 0) ? (val - valPrev) / valPrev : 0;
    if (TMath::Abs(relDiff) > thrBad) {
      vIndexBad.push_back(i - 1);
    } else if (TMath::Abs(relDiff) > thrMedium) {
      vIndexMedium.push_back(i - 1);
    }
  }
  return 0;
}

std::string RawDataReaderCheck::getAcceptedType() { return "TH1"; }

void RawDataReaderCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::shared_ptr<TLatex> msg;
  if (mo->getName() == "bcMTVX") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    h->GetXaxis()->SetTitle("BC");
    if (checkResult != Quality::Null)
      msg = std::make_shared<TLatex>(0.4, 0.85, Form("Quality: %s", (checkResult.getName()).c_str()));
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

    if (checkResult == Quality::Null) {
      msg = std::make_shared<TLatex>(0.4, 0.8, Form("Check was not performed, LHC information not available"));
      msg->SetTextColor(kBlack);
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
    } else {
      msg = std::make_shared<TLatex>(0.4, 0.8, Form("Number of good BC: %lu", vGoodBC.size()));
      msg->SetTextColor(kBlack);
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());

      msg = std::make_shared<TLatex>(0.4, 0.75, Form("Number of medium BC: %lu", vMediumBC.size()));
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());

      msg = std::make_shared<TLatex>(0.4, 0.7, Form("Number of bad BC: %lu", vBadBC.size()));
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());

      msg = std::make_shared<TLatex>(0.4, 0.65, Form("Threshold : %f", mThreshold));
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
    }
    h->SetStats(kFALSE);
    h->GetYaxis()->SetRangeUser(0, h->GetMaximum() * 1.5);
  } else {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    h->SetStats(kFALSE);
    msg = std::make_shared<TLatex>(0.45, 0.8, Form("Quality: %s", (checkResult.getName()).c_str()));
    std::string groupName = "Input";
    std::string relativeness = "relative";
    if (!flagInput) {
      groupName = "Class";
    }
    if (!flagRatio) {
      relativeness = "absolute";
    }
    if (flagInput) {
      for (size_t i = 0; i < h->GetXaxis()->GetNbins(); i++) {
        h->GetXaxis()->SetBinLabel(i + 1, ctpinputs[i]);
      }
      h->GetXaxis()->SetLabelSize(0.045);
      h->GetXaxis()->LabelsOption("v");
    }
    if (checkResult == Quality::Bad) {
      msg->SetTextColor(kRed);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with big %s rate change: %lu", groupName.c_str(), relativeness.c_str(), vIndexBad.size()));
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      for (size_t i = 0; i < vIndexBad.size(); i++) {
        if (flagInput) {
          msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("Check %s %s", ctpinputs[vIndexBad[i]], groupName.c_str()));
        } else {
          msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("Check %s with Index: %d", groupName.c_str(), vIndexBad[i]));
        }
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
      if (vIndexMedium.size() > 0) {
        msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with medium %s rate change: %lu", groupName.c_str(), relativeness.c_str(), vIndexMedium.size()));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
    } else if (checkResult == Quality::Medium) {
      msg->SetTextColor(kOrange);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      if (vIndexMedium.size() > 0) {
        msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with medium %s rate change: %lu", groupName.c_str(), relativeness.c_str(), vIndexMedium.size()));
      }
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      for (size_t i = 0; i < vIndexMedium.size(); i++) {
        msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("%s with Index: %d", groupName.c_str(), vIndexMedium[i]));
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

int RawDataReaderCheck::getNumberFilledBins(TH1F* hist)
{

  int nBins = hist->GetXaxis()->GetNbins();
  int filledBins = 0;
  for (int i = 0; i < nBins; i++) {
    if (hist->GetBinContent(i + 1) > 0) {
      filledBins++;
    }
  }
  return filledBins;
}
Quality RawDataReaderCheck::SetQualityResult(std::vector<int>& vBad, std::vector<int>& vMedium)
{

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
void RawDataReaderCheck::ClearIndexVectors()
{
  vBadBC.clear();
  vMediumBC.clear();
  vGoodBC.clear();
  vIndexBad.clear();
  vIndexMedium.clear();
}
void RawDataReaderCheck::startOfActivity(const core::Activity& activity)
{
  ILOG(Info, Support) << "RawDataReaderCheck::start : " << activity.mId << ENDM;
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
