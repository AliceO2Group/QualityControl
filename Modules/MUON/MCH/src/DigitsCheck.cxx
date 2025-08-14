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
/// \file   DigitsCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCH/DigitsCheck.h"
#include "MUONCommon/Helpers.h"
#include "QualityControl/ReferenceUtils.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TList.h>
#include <TLine.h>
#include <TPaveLabel.h>
#include <fstream>
#include <string>

using namespace std;
using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

void DigitsCheck::configure()
{
}

void DigitsCheck::startOfActivity(const Activity& activity)
{
  // input histogram names customization
  mMeanRateHistName = getConfigurationParameter<std::string>(mCustomParameters, "MeanRateHistName", mMeanRateHistName, activity);
  mGoodChanFracHistName = getConfigurationParameter<std::string>(mCustomParameters, "GoodChanFracHistName", mGoodChanFracHistName, activity);

  mMeanRatePerSolarHistName = getConfigurationParameter<std::string>(mCustomParameters, "MeanRatePerSolarHistName", mMeanRatePerSolarHistName, activity);
  mGoodChanFracPerSolarHistName = getConfigurationParameter<std::string>(mCustomParameters, "GoodChanFracPerSolarHistName", mGoodChanFracPerSolarHistName, activity);

  mMeanRateRefCompHistName = getConfigurationParameter<std::string>(mCustomParameters, "MeanRateRefCompHistName", mMeanRateRefCompHistName, activity);
  mGoodChanFracRefCompHistName = getConfigurationParameter<std::string>(mCustomParameters, "GoodChanFracRefCompHistName", mGoodChanFracRefCompHistName, activity);

  mMeanRatePerSolarRefCompHistName = getConfigurationParameter<std::string>(mCustomParameters, "MeanRatePerSolarRefCompHistName", mMeanRatePerSolarRefCompHistName, activity);
  mGoodChanFracPerSolarRefCompHistName = getConfigurationParameter<std::string>(mCustomParameters, "GoodChanFracPerSolarRefCompHistName", mGoodChanFracPerSolarRefCompHistName, activity);

  // threshold customization
  getThresholdsPerStation(mCustomParameters, activity, "MinRate", mMinRatePerStation, mMinRate);
  getThresholdsPerStation(mCustomParameters, activity, "MaxRate", mMaxRatePerStation, mMaxRate);
  mMinRateRatio = getConfigurationParameter<double>(mCustomParameters, "MinRateRatio", mMinRateRatio, activity);

  mMinRatePerSolar = getConfigurationParameter<double>(mCustomParameters, "MinRatePerSolar", mMinRatePerSolar, activity);
  mMaxRatePerSolar = getConfigurationParameter<double>(mCustomParameters, "MaxRatePerSolar", mMaxRatePerSolar, activity);
  mMinRateRatioPerSolar = getConfigurationParameter<double>(mCustomParameters, "MinRateRatioPerSolar", mMinRateRatioPerSolar, activity);

  getThresholdsPerStation(mCustomParameters, activity, "MinGoodFraction", mMinGoodFractionPerStation, mMinGoodFraction);
  mMinGoodFractionRatio = getConfigurationParameter<double>(mCustomParameters, "MinGoodFractionRatio", mMinGoodFractionRatio, activity);

  mMinGoodFractionPerSolar = getConfigurationParameter<double>(mCustomParameters, "MinGoodFractionPerSolar", mMinGoodFractionPerSolar, activity);
  mMinGoodFractionRatioPerSolar = getConfigurationParameter<double>(mCustomParameters, "MinGoodFractionRatioPerSolar", mMinGoodFractionRatioPerSolar, activity);

  mMaxBadST12 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST12", mMaxBadST12, activity);
  mMaxBadST345 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST345", mMaxBadST345, activity);

  mRatePlotScaleMin = getConfigurationParameter<double>(mCustomParameters, "RatePlotScaleMin", mRatePlotScaleMin, activity);
  mRatePlotScaleMax = getConfigurationParameter<double>(mCustomParameters, "RatePlotScaleMax", mRatePlotScaleMax, activity);
  mRateRatioPlotScaleRange = getConfigurationParameter<double>(mCustomParameters, "RateRatioPlotScaleRange", mRateRatioPlotScaleRange, activity);
  mRateRatioPerSolarPlotScaleRange = getConfigurationParameter<double>(mCustomParameters, "RateRatioPerSolarPlotScaleRange", mRateRatioPerSolarPlotScaleRange, activity);
  mGoodFractionRatioPlotScaleRange = getConfigurationParameter<double>(mCustomParameters, "GoodFractionRatioPlotScaleRange", mGoodFractionRatioPlotScaleRange, activity);
  mGoodFractionRatioPerSolarPlotScaleRange = getConfigurationParameter<double>(mCustomParameters, "GoodFractionRatioPerSolarPlotScaleRange", mGoodFractionRatioPerSolarPlotScaleRange, activity);

  mQualityChecker.mMaxBadST12 = mMaxBadST12;
  mQualityChecker.mMaxBadST345 = mMaxBadST345;
}

template <typename Lambda>
std::array<Quality, getNumDE()> checkPlot(TH1* h, Lambda check)
{
  std::array<Quality, getNumDE()> result;
  std::fill(result.begin(), result.end(), Quality::Null);

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    int chamberId = (de - 100) / 100;
    int stationId = chamberId / 2;

    int deId = getDEindex(de);
    if (deId < 0) {
      continue;
    }
    int bin = deId + 1;

    double val = h->GetBinContent(bin);
    if (check(val, stationId)) {
      result[deId] = Quality::Good;
    } else {
      result[deId] = Quality::Bad;
    }
  }

  return result;
}

std::array<Quality, getNumDE()> DigitsCheck::checkMeanRates(TH1* h)
{
  auto checkFunction = [&](double val, int station) -> bool {
    auto minRate = mMinRate;
    auto maxRate = mMaxRate;
    if (station >= 0 && station < 5) {
      if (mMinRatePerStation[station]) {
        minRate = mMinRatePerStation[station].value();
      }
      if (mMaxRatePerStation[station]) {
        maxRate = mMaxRatePerStation[station].value();
      }
    }
    return (val >= minRate && val <= maxRate);
  };
  return checkPlot(h, checkFunction);
}

std::array<Quality, getNumDE()> DigitsCheck::checkMeanRateRatios(TH1* h)
{
  auto checkFunction = [&](double val, int /*station*/) -> bool {
    auto minRateRatio = mMinRateRatio;
    return (val >= minRateRatio);
  };
  return checkPlot(h, checkFunction);
}

std::array<Quality, getNumDE()> DigitsCheck::checkBadChannels(TH1* h)
{
  auto checkFunction = [&](double val, int station) -> bool {
    auto minGoodFraction = mMinGoodFraction;
    if (station >= 0 && station < 5) {
      if (mMinGoodFractionPerStation[station]) {
        minGoodFraction = mMinGoodFractionPerStation[station].value();
      }
    }
    return (val >= minGoodFraction);
  };
  return checkPlot(h, checkFunction);
}

std::array<Quality, getNumDE()> DigitsCheck::checkBadChannelRatios(TH1* h)
{
  auto checkFunction = [&](double val, int /*station*/) -> bool {
    auto minGoodFractionRatio = mMinGoodFractionRatio;
    return (val >= minGoodFractionRatio);
  };
  return checkPlot(h, checkFunction);
}

void DigitsCheck::checkSolarMeanRates(TH1* h)
{
  for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
    int bin = solar + 1;
    double value = h->GetBinContent(bin);
    if (value >= mMinRatePerSolar && value <= mMaxRatePerSolar) {
      continue;
    }
    mSolarQuality[solar] = Quality::Bad;
  }
}

void DigitsCheck::checkSolarMeanRateRatios(TH1* h)
{
  for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
    int bin = solar + 1;
    double value = h->GetBinContent(bin);
    if (value >= mMinRateRatioPerSolar) {
      continue;
    }
    mSolarQuality[solar] = Quality::Bad;
  }
}

void DigitsCheck::checkSolarBadChannels(TH1* h)
{
  for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
    int bin = solar + 1;
    double value = h->GetBinContent(bin);
    if (value >= mMinGoodFractionPerSolar) {
      continue;
    }
    mSolarQuality[solar] = Quality::Bad;
  }
}

void DigitsCheck::checkSolarBadChannelRatios(TH1* h)
{
  for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
    int bin = solar + 1;
    double value = h->GetBinContent(bin);
    if (value >= mMinGoodFractionRatioPerSolar) {
      continue;
    }
    mSolarQuality[solar] = Quality::Bad;
  }
}

template <typename T>
static T* getHisto(TCanvas* c, std::string hname)
{
  if (!c) {
    return nullptr;
  }

  T* h = dynamic_cast<T*>(c->GetPrimitive(hname.c_str()));
  return h;
}

template <typename T>
static T* getHisto(std::shared_ptr<MonitorObject> mo)
{
  TObject* obj = dynamic_cast<TObject*>(mo->getObject());
  if (!obj) {
    return nullptr;
  }

  T* h{ nullptr };
  if (obj->InheritsFrom("TH1")) {
    h = dynamic_cast<T*>(obj);
  }

  if (obj->InheritsFrom("TCanvas")) {
    TCanvas* c = dynamic_cast<TCanvas*>(obj);
    h = getHisto<T>(c, mo->getName() + "Hist");
  }

  return h;
}

Quality DigitsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  mQualityChecker.reset();
  std::fill(mSolarQuality.begin(), mSolarQuality.end(), Quality::Good);

  for (auto& [moName, mo] : *moMap) {

    // DE histograms
    if (matchHistName(mo->getName(), mMeanRateHistName)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h) {
        auto q = checkMeanRates(h);
        mQualityChecker.addCheckResult(q);
      }
    }
    if (matchHistName(mo->getName(), mGoodChanFracHistName)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h) {
        auto q = checkBadChannels(h);
        mQualityChecker.addCheckResult(q);
      }
    }
    // comparisons with reference
    if (matchHistName(mo->getName(), mMeanRateRefCompHistName)) {
      TCanvas* canvas = dynamic_cast<TCanvas*>(mo->getObject());
      if (canvas) {
        auto ratioPlot = o2::quality_control::checker::getRatioPlotFromCanvas(canvas);
        if (ratioPlot) {
          auto q = checkMeanRateRatios(ratioPlot);
          mQualityChecker.addCheckResult(q);
        }
      }
    }
    if (matchHistName(mo->getName(), mGoodChanFracRefCompHistName)) {
      TCanvas* canvas = dynamic_cast<TCanvas*>(mo->getObject());
      if (canvas) {
        auto ratioPlot = o2::quality_control::checker::getRatioPlotFromCanvas(canvas);
        if (ratioPlot) {
          auto q = checkBadChannelRatios(ratioPlot);
          mQualityChecker.addCheckResult(q);
        }
      }
    }

    // SOLAR histograms
    if (matchHistName(mo->getName(), mMeanRatePerSolarHistName)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h) {
        checkSolarMeanRates(h);
      }
    }
    if (matchHistName(mo->getName(), mGoodChanFracPerSolarHistName)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h) {
        checkSolarBadChannels(h);
      }
    }
    // comparisons with reference
    if (matchHistName(mo->getName(), mMeanRatePerSolarRefCompHistName)) {
      TCanvas* canvas = dynamic_cast<TCanvas*>(mo->getObject());
      if (canvas) {
        auto ratioPlot = o2::quality_control::checker::getRatioPlotFromCanvas(canvas);
        if (ratioPlot) {
          checkSolarMeanRateRatios(ratioPlot);
        }
      }
    }
    if (matchHistName(mo->getName(), mGoodChanFracPerSolarRefCompHistName)) {
      TCanvas* canvas = dynamic_cast<TCanvas*>(mo->getObject());
      if (canvas) {
        auto ratioPlot = o2::quality_control::checker::getRatioPlotFromCanvas(canvas);
        if (ratioPlot) {
          checkSolarBadChannelRatios(ratioPlot);
        }
      }
    }
  }

  return mQualityChecker.getQuality();
}

static void updateTitle(TH1* hist, std::string suffix)
{
  if (!hist) {
    return;
  }
  TString title = hist->GetTitle();
  title.Append(" ");
  title.Append(suffix.c_str());
  hist->SetTitle(title);
}

void DigitsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if ((mo->getName().find("RefComp/") != std::string::npos)) {
    TCanvas* canvas = dynamic_cast<TCanvas*>(mo->getObject());
    if (!canvas) {
      return;
    }

    auto ratioPlot = o2::quality_control::checker::getRatioPlotFromCanvas(canvas);
    if (!ratioPlot) {
      return;
    }

    std::string messages;
    auto refCompPlots = o2::quality_control::checker::getPlotsFromCanvas(canvas, messages);

    double ratioPlotRange{ -1 };
    double ratioThreshold{ -1 };
    double plotScaleMin{ -1 };
    double plotScaleMax{ -1 };
    bool isSolar{ false };

    if (matchHistName(mo->getName(), mMeanRateRefCompHistName)) {
      ratioPlotRange = mRateRatioPlotScaleRange;
      ratioThreshold = mMinRateRatio;
      plotScaleMin = mRatePlotScaleMin;
      plotScaleMax = mRatePlotScaleMax;
      isSolar = false;
    } else if (matchHistName(mo->getName(), mMeanRatePerSolarRefCompHistName)) {
      ratioPlotRange = mRateRatioPerSolarPlotScaleRange;
      ratioThreshold = mMinRateRatioPerSolar;
      plotScaleMin = mRatePlotScaleMin;
      plotScaleMax = mRatePlotScaleMax;
      isSolar = true;
    } else if (matchHistName(mo->getName(), mGoodChanFracRefCompHistName)) {
      ratioPlotRange = mGoodFractionRatioPlotScaleRange;
      ratioThreshold = mMinGoodFractionRatio;
      plotScaleMin = 0;
      plotScaleMax = 1.05;
      isSolar = false;
    } else if (matchHistName(mo->getName(), mGoodChanFracPerSolarRefCompHistName)) {
      ratioPlotRange = mGoodFractionRatioPerSolarPlotScaleRange;
      ratioThreshold = mMinGoodFractionRatioPerSolar;
      plotScaleMin = 0;
      plotScaleMax = 1.05;
      isSolar = true;
    }

    if (ratioPlotRange < 0) {
      return;
    }

    ratioPlot->SetMinimum(1.0 - ratioPlotRange);
    ratioPlot->SetMaximum(1.0 + ratioPlotRange);
    ratioPlot->GetXaxis()->SetTickLength(0);

    if (isSolar) {
      addChamberDelimitersToSolarHistogram(ratioPlot);
      addSolarBinLabels(ratioPlot);
    } else {
      addChamberDelimiters(ratioPlot);
      addDEBinLabels(ratioPlot);
    }
    drawThreshold(ratioPlot, ratioThreshold);

    if (refCompPlots.first) {
      refCompPlots.first->SetMinimum(plotScaleMin);
      refCompPlots.first->SetMaximum(plotScaleMax);
      if (isSolar) {
        addChamberDelimitersToSolarHistogram(refCompPlots.first);
        addChamberLabelsForSolar(refCompPlots.first);
        addSolarBinLabels(refCompPlots.first);
        if (refCompPlots.second) {
          addSolarBinLabels(refCompPlots.second);
        }
      } else {
        addChamberDelimiters(refCompPlots.first);
        addChamberLabelsForDE(refCompPlots.first);
        addDEBinLabels(refCompPlots.first);
        if (refCompPlots.second) {
          addDEBinLabels(refCompPlots.second);
        }
      }
    }

    if (refCompPlots.second) {
      if (checkResult == Quality::Good) {
        refCompPlots.second->SetLineColor(kGreen + 2);
      } else if (checkResult == Quality::Bad) {
        refCompPlots.second->SetLineColor(kRed);
      } else if (checkResult == Quality::Medium) {
        refCompPlots.second->SetLineColor(kOrange - 3);
      } else if (checkResult == Quality::Null) {
        refCompPlots.second->SetLineColor(kViolet - 6);
      }
    }
    return;
  }

  if (mo->getName().find("Occupancy_Elec") != std::string::npos ||
      mo->getName().find("OccupancySignal_Elec") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    h->SetMinimum(mRatePlotScaleMin);
    h->SetMaximum(mRatePlotScaleMax);

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if ((mo->getName().find("Rate_ST12") != std::string::npos) ||
      (mo->getName().find("Rate_ST345") != std::string::npos) ||
      (mo->getName().find("Rate_XY") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());

    h->SetMinimum(mRatePlotScaleMin);
    h->SetMaximum(mRatePlotScaleMax);

    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if (mo->getName().find("MeanRate") != std::string::npos) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }

    double scaleMin{ mRatePlotScaleMin };
    double scaleMax{ mRatePlotScaleMax };
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);
    h->GetYaxis()->SetTitle("rate (kHz)");

    if (mo->getName().find("MeanRatePerSolar") != std::string::npos) {
      addChamberDelimitersToSolarHistogram(h, scaleMin, scaleMax);
      addChamberLabelsForSolar(h);
    } else {
      addChamberDelimiters(h, scaleMin, scaleMax);
      addChamberLabelsForDE(h);
    }

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mMeanRateHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThresholdsPerStation(h, mMinRatePerStation, mMinRate);
      drawThresholdsPerStation(h, mMaxRatePerStation, mMaxRate);
    }

    // also beautify the SOLAR plot
    if (matchHistName(mo->getName(), mMeanRatePerSolarHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThreshold(h, mMinRatePerSolar);
      drawThreshold(h, mMaxRatePerSolar);
    }
  }

  if (mo->getName().find("GoodChannelsFraction") != std::string::npos) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }

    float scaleMin{ 0 };
    float scaleMax{ 1.05 };
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);
    h->GetYaxis()->SetTitle("fraction");

    if (mo->getName().find("GoodChannelsFractionPerSolar") != std::string::npos) {
      addChamberDelimitersToSolarHistogram(h, scaleMin, scaleMax);
      addChamberLabelsForSolar(h);
    } else {
      addChamberDelimiters(h, scaleMin, scaleMax);
      addChamberLabelsForDE(h);
    }

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mGoodChanFracHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThresholdsPerStation(h, mMinGoodFractionPerStation, mMinGoodFraction);
    }

    // also beautify the SOLAR plot
    if (matchHistName(mo->getName(), mGoodChanFracPerSolarHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThreshold(h, mMinGoodFractionPerSolar);
    }
  }

  // update quality flags for each DE
  if (mo->getName().find("QualityFlagPerDE") != std::string::npos) {
    TH2F* h = getHisto<TH2F>(mo);
    if (!h) {
      return;
    }

    std::string badDEs;
    for (int deIndex = 0; deIndex < mQualityChecker.mQuality.size(); deIndex++) {
      float ybin = 0;
      if (mQualityChecker.mQuality[deIndex] == Quality::Good) {
        ybin = 3;
      }
      if (mQualityChecker.mQuality[deIndex] == Quality::Medium) {
        ybin = 2;
      }
      if (mQualityChecker.mQuality[deIndex] == Quality::Bad) {
        ybin = 1;
        std::string deIdStr = std::to_string(getDEFromIndex(deIndex));
        if (badDEs.empty()) {
          badDEs = deIdStr;
        } else {
          badDEs += std::string(" ") + deIdStr;
        }
      }

      h->SetBinContent(deIndex + 1, ybin, 1);
    }

    if (!badDEs.empty()) {
      std::string text = std::string("Bad DEs: ") + badDEs;
      TPaveLabel* label = new TPaveLabel(0.2, 0.8, 0.8, 0.88, text.c_str(), "blNDC");
      label->SetBorderSize(1);
      h->GetListOfFunctions()->Add(label);

      ILOG(Warning, Support) << "[DecodingCheck] " << text << ENDM;
    }
  }

  // update quality flags for each SOLAR
  if (mo->getName().find("QualityFlagPerSolar") != std::string::npos) {
    TH2F* h = getHisto<TH2F>(mo);
    if (!h) {
      return;
    }

    std::string badSolarBoards;
    for (int solar = 0; solar < mSolarQuality.size(); solar++) {
      float ybin = 0;
      if (mSolarQuality[solar] == Quality::Good) {
        ybin = 3;
      }
      if (mSolarQuality[solar] == Quality::Medium) {
        ybin = 2;
      }
      if (mSolarQuality[solar] == Quality::Bad) {
        ybin = 1;
        std::string solarId = std::to_string(getSolarIdFromIndex(solar));
        if (badSolarBoards.empty()) {
          badSolarBoards = solarId;
        } else {
          badSolarBoards += std::string(" ") + solarId;
        }
      }

      h->SetBinContent(solar + 1, ybin, 1);
    }

    if (!badSolarBoards.empty()) {
      std::string badSolarList = std::string("Bad SOLAR boards: ") + badSolarBoards;
      TPaveLabel* label = new TPaveLabel(0.2, 0.8, 0.8, 0.88, badSolarList.c_str(), "blNDC");
      label->SetBorderSize(1);
      h->GetListOfFunctions()->Add(label);

      ILOG(Warning, Support) << "[DigitsCheck] " << badSolarList << ENDM;
    }
  }
}

} // namespace o2::quality_control_modules::muonchambers
