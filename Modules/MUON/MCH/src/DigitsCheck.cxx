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
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TList.h>
#include <TLine.h>
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
  mMeanRateHistName = getConfigurationParameter<std::string>(mCustomParameters, "MeanRateHistName", mMeanRateHistName, activity);
  mGoodChanFracHistName = getConfigurationParameter<std::string>(mCustomParameters, "GoodChanFracHistName", mGoodChanFracHistName, activity);

  getThresholdsPerStation(mCustomParameters, activity, "MinRate", mMinRatePerStation, mMinRate);
  getThresholdsPerStation(mCustomParameters, activity, "MaxRate", mMaxRatePerStation, mMaxRate);
  getThresholdsPerStation(mCustomParameters, activity, "MinGoodFraction", mMinGoodFractionPerStation, mMinGoodFraction);

  mMaxBadST12 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST12", mMaxBadST12, activity);
  mMaxBadST345 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST345", mMaxBadST345, activity);

  mRatePlotScaleMin = getConfigurationParameter<double>(mCustomParameters, "RatePlotScaleMin", mRatePlotScaleMin, activity);
  mRatePlotScaleMax = getConfigurationParameter<double>(mCustomParameters, "RatePlotScaleMax", mRatePlotScaleMax, activity);

  mQualityChecker.mMaxBadST12 = mMaxBadST12;
  mQualityChecker.mMaxBadST345 = mMaxBadST345;
}

template <typename Lambda>
std::array<Quality, getNumDE()> checkPlot(TH1F* h, Lambda check)
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

std::array<Quality, getNumDE()> DigitsCheck::checkMeanRates(TH1F* h)
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

std::array<Quality, getNumDE()> DigitsCheck::checkBadChannels(TH1F* h)
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

  for (auto& [moName, mo] : *moMap) {

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
  }

  return mQualityChecker.getQuality();
}

std::string DigitsCheck::getAcceptedType() { return "TH1"; }

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

    addChamberDelimiters(h, scaleMin, scaleMax);
    addDEBinLabels(h);

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

    addChamberDelimiters(h, scaleMin, scaleMax);
    addDEBinLabels(h);

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
  }

  // update quality flags for each DE
  if (mo->getName().find("QualityFlagPerDE") != std::string::npos) {
    TH2F* h = getHisto<TH2F>(mo);
    if (!h) {
      return;
    }

    addChamberDelimiters(h);
    addDEBinLabels(h);

    for (int deId = 0; deId < mQualityChecker.mQuality.size(); deId++) {
      float ybin = 0;
      if (mQualityChecker.mQuality[deId] == Quality::Good) {
        ybin = 3;
      }
      if (mQualityChecker.mQuality[deId] == Quality::Medium) {
        ybin = 2;
      }
      if (mQualityChecker.mQuality[deId] == Quality::Bad) {
        ybin = 1;
      }

      h->SetBinContent(deId + 1, ybin, 1);
    }
  }
}

} // namespace o2::quality_control_modules::muonchambers
