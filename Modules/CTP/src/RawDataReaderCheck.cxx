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
#include "DataFormatsCTP/Configuration.h"
#include "DataFormatsCTP/RunManager.h"
#include <DataFormatsCTP/Configuration.h>
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include <DataFormatsCTP/Configuration.h>
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

  map<string, string> metadata; // can be empty

  auto lhcifdata = UserCodeInterface::retrieveConditionAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF", metadata, mTimestamp);
  if(lhcifdata == nullptr) {
    ILOG(Info, Support) << "LHC data not found for timestamp:" << mTimestamp << ENDM;
    return result;
  }
  auto bfilling = lhcifdata->getBunchFilling();
  std::vector<int> bcs = bfilling.getFilledBCs();
  std::bitset<o2::constants::lhc::LHCMaxBunches> lhcBC_bitset;
  lhcBC_bitset.reset();
  vBadBC.clear();
  vMediumBC.clear();
  vGoodBC.clear();
  vIndexBad.clear();
  vIndexMedium.clear();
  relativeRates = false;
  inputRates = false;

  for (auto const& bc : bcs) {
    lhcBC_bitset.set(bc, 1);
  }
  //int nhistos = (*moMap).size();
  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "bcMTVX") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        ILOG(Info, Support) << "histogram bcMTVX is not found for check" << ENDM;
        continue;
      }
      mThreshold = h->GetEntries() / getNumberFilledBins(h);
      mThreshold = mThreshold - sqrt(mThreshold);

      for (int i = 0; i < o2::constants::lhc::LHCMaxBunches; i++) {
        if (lhcBC_bitset[i]) {
          ILOG(Info, Support) << i << " ";
        }
        if (lhcBC_bitset[i] && h->GetBinContent(i + 1) <= mThreshold) {
          vMediumBC.push_back(i);
        } else if (!lhcBC_bitset[i] && h->GetBinContent(i + 1) > mThreshold) {
          vBadBC.push_back(i);
        } else if (lhcBC_bitset[i] && h->GetBinContent(i + 1) > mThreshold) {
          vGoodBC.push_back(i);
        }
      }
    } else {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        ILOG(Info, Support) << "histogram for input/class rate is not found for check" << ENDM;
        continue;
      }
      std::string moName = mo->getName();
      if (moName.find("Ratio") != std::string::npos) {
        relativeRates = true;
      }
      if (moName.find("input") != std::string::npos) {
        inputRates = true;
      }
      if (relativeRates && inputRates) {
        if (h->GetBinContent(3) != 0) {
          h->Scale(1. / h->GetBinContent(3));
        }
      } else if (relativeRates) {
        if (h->GetBinContent(mIndexMBclass) != 0) {
          h->Scale(1. / h->GetBinContent(mIndexMBclass));
        }
      }

      if (cycleCounter > 1) { // skipping the check for first two cycles, as the first one is arbitrary long
        TH1F* fHistDifference = (TH1F*)h->Clone();
        TH1F* fHistPrev;
        if (inputRates && !relativeRates) {
          if (fHistInputPrevious) {
            fHistPrev = (TH1F*)fHistInputPrevious->Clone();
          } else {
            fHistInputPrevious = (TH1F*)h->Clone();
            continue;
          }
        }
        if (inputRates && relativeRates) {
          if (fHistInputRatioPrevious) {
            fHistPrev = (TH1F*)fHistInputRatioPrevious->Clone();
          } else {
            fHistInputRatioPrevious = (TH1F*)h->Clone();
            continue;
          }
        }
        if (!inputRates && !relativeRates) {
          if (fHistClassesPrevious) {
            fHistPrev = (TH1F*)fHistClassesPrevious->Clone();
          } else {
            fHistClassesPrevious = (TH1F*)h->Clone();
            continue;
          }
        }
        if (!inputRates && relativeRates) {
          if (fHistClassRatioPrevious) {
            fHistPrev = (TH1F*)fHistClassRatioPrevious->Clone();
          } else {
            fHistClassRatioPrevious = (TH1F*)h->Clone();
            continue;
          }
        }
        if (!relativeRates) {
          fHistDifference->Scale(1. / (mCycleDuration * mFraction));
          fHistPrev->Scale(1. / (mCycleDuration * mFraction));
        }
        fHistDifference->Add(fHistPrev, -1); // Calculate relative difference w.r.t. rate in previous cycle
        fHistDifference->Divide(fHistPrev);

        float thrBad = mThresholdRateBad;
        float thrMedium = mThresholdRateMedium;
        if (relativeRates) {
          thrBad = mThresholdRateRatioBad;
          thrMedium = mThresholdRateRatioMedium;
        }
        for (size_t i = 1; i < fHistDifference->GetXaxis()->GetNbins() + 1; i++) { // Check how many inputs/classes changed more than a threshold value
          if (TMath::Abs(fHistDifference->GetBinContent(i)) > thrBad) {
            vIndexBad.push_back(i);
          } else if (TMath::Abs(fHistDifference->GetBinContent(i)) > thrMedium) {
            vIndexMedium.push_back(i);
          }
        }
        delete fHistDifference;
        delete fHistPrev;
      }
      if (inputRates && !relativeRates) {
        delete fHistInputPrevious;
        fHistInputPrevious = (TH1F*)h->Clone();
      }
      if (inputRates && relativeRates) {
        delete fHistInputRatioPrevious;
        fHistInputRatioPrevious = (TH1F*)h->Clone();
      }
      if (!inputRates && !relativeRates) {
        delete fHistClassesPrevious;
        fHistClassesPrevious = (TH1F*)h->Clone();
      }
      if (!inputRates && relativeRates) {
        delete fHistClassRatioPrevious;
        fHistClassRatioPrevious = (TH1F*)h->Clone();
      }
      if (!relativeRates && inputRates) {
        cycleCounter++;
      }
    }
  }
  if ((vBadBC.size() > 0) || (vIndexBad.size() > 0)) {
    result = Quality::Bad;
  } else if ((vMediumBC.size()  > 0) || (vIndexMedium.size() > 0)){
    result = Quality::Medium;
  } else {
    result = Quality::Good;
  }
  return result;
}

std::string RawDataReaderCheck::getAcceptedType() { return "TH1"; }

void RawDataReaderCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::shared_ptr<TLatex> msg;
  if (mo->getName() == "bcMTVX") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    msg = std::make_shared<TLatex>(0.5, 0.8, Form("Overall Quality: %s", (checkResult.getName()).c_str()));
    if (checkResult == Quality::Bad) {
      msg->SetTextColor(kRed);
    } else if (checkResult == Quality::Medium) {
      msg->SetTextColor(kOrange);
    } else {
      msg->SetTextColor(kGreen + 1);
    }
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.75, Form("Number of good BC: %lu", vGoodBC.size()));
    msg->SetTextColor(kBlack);
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.7, Form("Number of medium BC: %lu", vMediumBC.size()));
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.65, Form("Number of bad BC: %lu", vBadBC.size()));
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.6, Form("Threshold : %f", mThreshold));
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    h->GetYaxis()->SetRangeUser(0, h->GetMaximum() * 1.5);
  } else {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    h->SetStats(kFALSE);
    msg = std::make_shared<TLatex>(0.45, 0.8, Form("Overall Quality: %s", (checkResult.getName()).c_str()));
    std::string groupName = "Input";
    std::string relativeness = "relative";
    if (!inputRates) {
      groupName = "Class";
    }
    if (!relativeRates) {
      relativeness = "absolute";
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
        msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("Check %s with Index: %d", groupName.c_str(), vIndexBad[i]));
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

void RawDataReaderCheck::startOfActivity(const core::Activity& activity)
{
  ILOG(Debug, Devel) << "RawDataReaderCheck::start : " << activity.mId << ENDM;
  mRunNumber = activity.mId;
  mTimestamp = activity.mValidity.getMin();
  cycleCounter = 0;

  std::string MBclassName = mCustomParameters["MBclassName"];
  if (MBclassName.empty()) {
    MBclassName = "CMTVX-B-NOPF";
  }
  std::string run = std::to_string(mRunNumber);
  CTPRunManager::setCCDBHost("https://alice-ccdb.cern.ch");
  o2::ctp::CTPConfiguration CTPconfig = CTPRunManager::getConfigFromCCDB(mTimestamp, run);

  // get the index of the MB reference class
  std::vector<o2::ctp::CTPClass> ctpcls = CTPconfig.getCTPClasses();
  std::vector<int> clslist = CTPconfig.getTriggerClassList();
  for (size_t i = 0; i < clslist.size(); i++) {
    if (ctpcls[i].name.find(MBclassName) != std::string::npos) {
      mIndexMBclass = ctpcls[i].descriptorIndex + 1;
      break;
    }
  }
  if (mIndexMBclass == -1) {
    mIndexMBclass = 1;
  }
}

} // namespace o2::quality_control_modules::ctp
