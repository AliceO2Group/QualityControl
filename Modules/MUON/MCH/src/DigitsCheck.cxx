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
#include <MCHConstants/DetectionElements.h>
#include <MCHRawElecMap/Mapper.h>
#include "QualityControl/MonitorObject.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TList.h>
#include <TLine.h>
#include <fstream>
#include <string>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

void DigitsCheck::configure()
{
  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();

  if (auto param = mCustomParameters.find("MeanRateHistName"); param != mCustomParameters.end()) {
    mMeanRateHistName = param->second;
  }
  if (auto param = mCustomParameters.find("MeanRateRatioHistName"); param != mCustomParameters.end()) {
    mMeanRateRatioHistName = param->second;
  }
  if (auto param = mCustomParameters.find("GoodChanFracHistName"); param != mCustomParameters.end()) {
    mGoodChanFracHistName = param->second;
  }
  if (auto param = mCustomParameters.find("GoodChanFracRatioHistName"); param != mCustomParameters.end()) {
    mGoodChanFracRatioHistName = param->second;
  }

  if (auto param = mCustomParameters.find("MinRate"); param != mCustomParameters.end()) {
    mMinRate = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxRate"); param != mCustomParameters.end()) {
    mMaxRate = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxRateDelta"); param != mCustomParameters.end()) {
    mMaxRateDelta = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinGoodFraction"); param != mCustomParameters.end()) {
    mMinGoodFraction = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxGoodFractionDelta"); param != mCustomParameters.end()) {
    mMaxGoodFractionDelta = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("RatePlotScaleMin"); param != mCustomParameters.end()) {
    mRatePlotScaleMin = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("RatePlotScaleMax"); param != mCustomParameters.end()) {
    mRatePlotScaleMax = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxBadDE_ST12"); param != mCustomParameters.end()) {
    mMaxBadST12 = std::stoi(param->second);
  }
  if (auto param = mCustomParameters.find("MaxBadDE_ST345"); param != mCustomParameters.end()) {
    mMaxBadST345 = std::stoi(param->second);
  }

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
    int chamberIdInStation = chamberId % 2;

    int deId = getDEindex(de);
    if (deId < 0) {
      continue;
    }
    int bin = deId + 1;

    double val = h->GetBinContent(bin);
    if (check(val)) {
      result[deId] = Quality::Good;
    } else {
      result[deId] = Quality::Bad;
    }
  }

  return result;
}

std::array<Quality, getNumDE()> DigitsCheck::checkMeanRates(TH1F* h)
{
  return checkPlot(h, [&](double val) -> bool { return (val >= mMinRate && val <= mMaxRate); });
}

std::array<Quality, getNumDE()> DigitsCheck::checkMeanRatesRatio(TH1F* h)
{
  return checkPlot(h, [&](double val) -> bool { return (std::abs(val - 1) <= mMaxRateDelta); });
}

std::array<Quality, getNumDE()> DigitsCheck::checkBadChannels(TH1F* h)
{
  return checkPlot(h, [&](double val) -> bool { return (val >= mMinGoodFraction); });
}

std::array<Quality, getNumDE()> DigitsCheck::checkBadChannelsRatio(TH1F* h)
{
  return checkPlot(h, [&](double val) -> bool { return (std::abs(val - 1) <= mMaxGoodFractionDelta); });
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

    if (matchHistName(mo->getName(), mMeanRateRatioHistName)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h && h->GetEntries() > 0) {
        auto q = checkMeanRatesRatio(h);
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

    if (matchHistName(mo->getName(), mGoodChanFracRatioHistName)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h && h->GetEntries() > 0) {
        auto q = checkBadChannelsRatio(h);
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

static void updateTitle(TCanvas* c, std::string suffix)
{
  if (!c) {
    return;
  }

  TObject* obj;
  TIter next(c->GetListOfPrimitives());
  while ((obj = next())) {
    if (obj->InheritsFrom("TH1")) {
      TH1* hist = dynamic_cast<TH1*>(obj);
      updateTitle(hist, suffix);
    }
  }
}

static std::string getCurrentTime()
{
  time_t t;
  time(&t);

  struct tm* tmp;
  tmp = localtime(&t);

  char timestr[500];
  strftime(timestr, sizeof(timestr), "(%d/%m/%Y - %R)", tmp);

  std::string result = timestr;
  return result;
}

void DigitsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);
  updateTitle(dynamic_cast<TCanvas*>(mo->getObject()), currentTime);

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

    float scaleMin{ 0 };
    float scaleMax{ 0 };
    if (mo->getName().find("MeanRateRefRatio") != std::string::npos) {
      scaleMin = 1.0 - mMaxRateDelta * 2.0;
      scaleMax = 1.0 + mMaxRateDelta * 2.0;
      h->GetYaxis()->SetTitle("rate / reference");
    } else {
      scaleMin = mRatePlotScaleMin;
      scaleMax = mRatePlotScaleMax;
      h->GetYaxis()->SetTitle("rate (kHz)");
    }
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);

    addChamberDelimiters(h, scaleMin, scaleMax);

    if (mo->getName().find("MeanRateRefRatio") != std::string::npos) {
      // draw horizontal limits
      TLine* l = new TLine(0, 1, h->GetXaxis()->GetXmax(), 1);
      l->SetLineColor(kBlack);
      l->SetLineStyle(kDotted);
      h->GetListOfFunctions()->Add(l);

      if (h->GetEntries() > 0) {
        l = new TLine(0, 1.0 - mMaxRateDelta, h->GetXaxis()->GetXmax(), 1.0 - mMaxRateDelta);
        l->SetLineColor(kBlue);
        l->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(l);

        l = new TLine(0, 1.0 + mMaxRateDelta, h->GetXaxis()->GetXmax(), 1.0 + mMaxRateDelta);
        l->SetLineColor(kBlue);
        l->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(l);
      }
    } else {
      // draw horizontal limits
      TLine* l = new TLine(0, mMinRate, h->GetXaxis()->GetXmax(), mMinRate);
      l->SetLineColor(kBlue);
      l->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(l);

      l = new TLine(0, mMaxRate, h->GetXaxis()->GetXmax(), mMaxRate);
      l->SetLineColor(kBlue);
      l->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(l);
    }

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if (mo->getName().find("GoodChannelsFraction") != std::string::npos) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }

    float scaleMin{ 0 };
    float scaleMax{ 0 };
    if (mo->getName().find("GoodChannelsFractionRefRatio") != std::string::npos) {
      scaleMin = 1.0 - mMaxGoodFractionDelta * 2.0;
      scaleMax = 1.0 + mMaxGoodFractionDelta * 2.0;
      h->GetYaxis()->SetTitle("fraction / reference");
    } else {
      scaleMin = 0;
      scaleMax = 1.05;
      h->GetYaxis()->SetTitle("fraction");
    }
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);

    addChamberDelimiters(h, scaleMin, scaleMax);

    if (mo->getName().find("GoodChannelsFractionRefRatio") != std::string::npos) {
      // draw horizontal limits
      TLine* l = new TLine(0, 1, h->GetXaxis()->GetXmax(), 1);
      l->SetLineColor(kBlack);
      l->SetLineStyle(kDotted);
      h->GetListOfFunctions()->Add(l);

      if (h->GetEntries() > 0) {
        l = new TLine(0, 1.0 - mMaxGoodFractionDelta, h->GetXaxis()->GetXmax(), 1.0 - mMaxGoodFractionDelta);
        l->SetLineColor(kBlue);
        l->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(l);

        l = new TLine(0, 1.0 + mMaxGoodFractionDelta, h->GetXaxis()->GetXmax(), 1.0 + mMaxGoodFractionDelta);
        l->SetLineColor(kBlue);
        l->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(l);
      }
    } else {
      // draw horizontal limits
      TLine* l = new TLine(0, mMinGoodFraction, h->GetXaxis()->GetXmax(), mMinGoodFraction);
      l->SetLineColor(kBlue);
      l->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(l);
    }

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  // update quality flags for each DE
  if (mo->getName().find("QualityFlagPerDE") != std::string::npos) {
    TH2F* h = getHisto<TH2F>(mo);
    if (!h) {
      return;
    }

    addChamberDelimiters(h);

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
