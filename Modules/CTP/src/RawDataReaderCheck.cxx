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
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include <DataFormatsCTP/Configuration.h>
#include <DataFormatsParameters/GRPLHCIFData.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include <DetectorsBase/GRPGeomHelper.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::ctp;

namespace o2::quality_control_modules::ctp
{

void RawDataReaderCheck::configure() {}

Quality RawDataReaderCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  map<string, string> metadata; // can be empty

  auto lhcifdata = UserCodeInterface::retrieveConditionAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF", metadata, mTimestamp);
  auto bfilling = lhcifdata->getBunchFilling();

  std::vector<int> bcs = bfilling.getFilledBCs();
  o2::ctp::BCMask* bcmask;
  std::bitset<o2::constants::lhc::LHCMaxBunches> lhcBC_bitset = bcmask->BCmask;
  lhcBC_bitset.reset();
  vBadBC.clear();
  vMediumBC.clear();
  vGoodBC.clear();
  vIndexBad.clear();
  vIndexMedium.clear();

  for (auto const& bc : bcs) {
    lhcBC_bitset.set(bc, 1);
  }

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "bcMTVX") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mThreshold = h->GetEntries() / getNumberFilledBins(h);
      mThreshold = mThreshold - sqrt(mThreshold);

      for (int i = 0; i < o2::constants::lhc::LHCMaxBunches; i++) {
        if (lhcBC_bitset[i])
          ILOG(Info, Support) << i << " ";
        if (lhcBC_bitset[i] && h->GetBinContent(i + 1) <= mThreshold) {
          vMediumBC.push_back(i);
        } else if (!lhcBC_bitset[i] && h->GetBinContent(i + 1) > mThreshold) {
          vBadBC.push_back(i);
        } else if (lhcBC_bitset[i] && h->GetBinContent(i + 1) > mThreshold) {
          vGoodBC.push_back(i);
        }
      }
      if (vBadBC.size() > 0)
        result = Quality::Bad;
      else if (vMediumBC.size() > 0)
        result = Quality::Medium;
      else
        result = Quality::Good;
    }
    if (mo->getName() == "inputs" || mo->getName() == "classes") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (mo->getName() == "inputs" && (!fHistInputPrevious || inputCycleCounter < 2)) {
        fHistInputPrevious = (TH1F*)h->Clone();
        inputCycleCounter++;
      } else if (mo->getName() == "classes" && (!fHistClassesPrevious || classCycleCounter < 2)) {
        fHistClassesPrevious = (TH1F*)h->Clone();
        classCycleCounter++;
      } else {
        TH1F* fHistDifference = (TH1F*)h->Clone();
        TH1F* fHistPrev;
        if (mo->getName() == "inputs")
          fHistPrev = (TH1F*)fHistInputPrevious->Clone();
        if (mo->getName() == "classes")
          fHistPrev = (TH1F*)fHistClassesPrevious->Clone();
        fHistDifference->Scale(1. / 60);
        fHistPrev->Scale(1. / 60);
        fHistDifference->Add(fHistPrev, -1);
        fHistDifference->Divide(fHistPrev);

        for (size_t i = 1; i < fHistDifference->GetXaxis()->GetNbins() + 1; i++) {
          if (TMath::Abs(fHistDifference->GetBinContent(i)) > 0.15)
            vIndexBad.push_back(i);
          else if (TMath::Abs(fHistDifference->GetBinContent(i)) > 0.1)
            vIndexMedium.push_back(i);
        }

        if (mo->getName() == "inputs") {
          fHistInputPrevious = (TH1F*)h->Clone();
          inputCycleCounter++;
        }
        if (mo->getName() == "classes") {
          fHistClassesPrevious = (TH1F*)h->Clone();
          classCycleCounter++;
        }
      }
      if (vIndexBad.size() > 0)
        result = Quality::Bad;
      else if (vIndexMedium.size() > 0)
        result = Quality::Medium;
      else
        result = Quality::Good;
    }
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
    if (checkResult == Quality::Bad)
      msg->SetTextColor(kRed);
    else if (checkResult == Quality::Medium)
      msg->SetTextColor(kOrange);
    else
      msg->SetTextColor(kGreen + 1);
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
  }
  if (mo->getName() == "inputs" || mo->getName() == "classes") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    h->SetStats(kFALSE);
    msg = std::make_shared<TLatex>(0.45, 0.8, Form("Overall Quality: %s", (checkResult.getName()).c_str()));
    if (checkResult == Quality::Bad) {
      msg->SetTextColor(kRed);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with big rate change: %lu", (mo->getName()).c_str(), vIndexBad.size()));
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      for (size_t i = 0; i < vIndexBad.size(); i++) {
        if (mo->getName() == "inputs")
          msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("Check Input with Index: %d", vIndexBad[i]));
        if (mo->getName() == "classes")
          msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("Check Class with Index: %d", vIndexBad[i]));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
      if (vIndexMedium.size() > 0) {
        msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with medium rate change: %lu", (mo->getName()).c_str(), vIndexMedium.size()));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
    } else if (checkResult == Quality::Medium) {
      msg->SetTextColor(kOrange);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      msg = std::make_shared<TLatex>(0.45, 0.75, Form("Number of %s with medium rate change: %lu", (mo->getName()).c_str(), vIndexMedium.size()));
      msg->SetTextSize(0.03);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      for (size_t i = 0; i < vIndexMedium.size(); i++) {
        if (mo->getName() == "inputs")
          msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("Input with Index: %d", vIndexMedium[i]));
        if (mo->getName() == "classes")
          msg = std::make_shared<TLatex>(0.45, 0.7 - i * 0.05, Form("Class with Index: %d", vIndexMedium[i]));
        msg->SetTextSize(0.03);
        msg->SetNDC();
        h->GetListOfFunctions()->Add(msg->Clone());
      }
    } else {
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
    if (hist->GetBinContent(i + 1) > 0)
      filledBins++;
  }
  return filledBins;
}

void RawDataReaderCheck::startOfActivity(const core::Activity& activity)
{
  ILOG(Debug, Devel) << "RawDataReaderCheck::start : " << activity.mId << ENDM;
  mRunNumber = activity.mId;
  mTimestamp = activity.mValidity.getMin();
  inputCycleCounter = 0;
  classCycleCounter = 0;
}

} // namespace o2::quality_control_modules::ctp
