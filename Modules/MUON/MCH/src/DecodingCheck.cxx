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
/// \file   DecodingCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCHMappingInterface/Segmentation.h"
#include "MCH/DecodingCheck.h"
#include "MCH/Helpers.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TLine.h>
#include <fstream>
#include <string>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

void DecodingCheck::configure()
{
  if (auto param = mCustomParameters.find("MinGoodErrorFrac"); param != mCustomParameters.end()) {
    mMinGoodErrorFrac = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinGoodSyncFrac"); param != mCustomParameters.end()) {
    mMinGoodSyncFrac = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("GoodFracHistName"); param != mCustomParameters.end()) {
    mGoodFracHistName = param->second;
  }
  if (auto param = mCustomParameters.find("SyncFracHistName"); param != mCustomParameters.end()) {
    mSyncFracHistName = param->second;
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

Quality DecodingCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  std::array<Quality, getNumDE()> errorsQuality;
  std::fill(errorsQuality.begin(), errorsQuality.end(), Quality::Null);
  std::array<Quality, getNumDE()> syncQuality;
  std::fill(syncQuality.begin(), syncQuality.end(), Quality::Null);

  mQualityChecker.reset();

  for (auto& [moName, mo] : *moMap) {

    if (matchHistName(mo->getName(), mGoodFracHistName)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      for (int deId = 0; deId < getNumDE(); deId++) {
        double val = h->GetBinContent(deId + 1);
        if (val < mMinGoodErrorFrac) {
          errorsQuality[deId] = Quality::Bad;
        } else {
          errorsQuality[deId] = Quality::Good;
        }
      }
      mQualityChecker.addCheckResult(errorsQuality);
    }

    if (matchHistName(mo->getName(), mSyncFracHistName)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      for (int deId = 0; deId < getNumDE(); deId++) {
        double val = h->GetBinContent(deId + 1);
        if (val < mMinGoodSyncFrac) {
          syncQuality[deId] = Quality::Bad;
        } else {
          syncQuality[deId] = Quality::Good;
        }
      }
      mQualityChecker.addCheckResult(syncQuality);
    }
  }
  return mQualityChecker.getQuality();
}

std::string DecodingCheck::getAcceptedType() { return "TH1"; }

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

void DecodingCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if (mo->getName().find("DecodingErrorsPerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    if (!h) {
      return;
    }

    // disable ticks on horizontal axis
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);

    TText* xtitle = new TText();
    xtitle->SetNDC();
    xtitle->SetText(0.87, 0.05, "chamber #");
    xtitle->SetTextSize(15);
    h->GetListOfFunctions()->Add(xtitle);

    float scaleMin = h->GetYaxis()->GetXmin();
    float scaleMax = h->GetYaxis()->GetXmax();

    // draw chamber delimiters
    float xpos = 0;
    for (int ch = 1; ch <= 9; ch++) {
      xpos += getNumDEinChamber(ch - 1);
      TLine* delimiter = new TLine(xpos, scaleMin, xpos, scaleMax);
      delimiter->SetLineColor(kBlack);
      delimiter->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(delimiter);
    }

    // draw x-axis labels
    xpos = 0;
    for (int ch = 0; ch <= 9; ch += 1) {
      float x1 = xpos;
      xpos += getNumDEinChamber(ch);
      float x2 = (ch < 9) ? xpos : h->GetXaxis()->GetXmax();
      float x0 = 0.8 * (x1 + x2) / (2 * h->GetXaxis()->GetXmax()) + 0.1;
      float y0 = 0.08;
      TText* label = new TText();
      label->SetNDC();
      label->SetText(x0, y0, TString::Format("%d", ch + 1));
      label->SetTextSize(15);
      label->SetTextAlign(22);
      h->GetListOfFunctions()->Add(label);
    }
  }

  if (mo->getName().find("GoodBoardsFractionPerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (!h) {
      return;
    }

    float scaleMin{ 0 };
    float scaleMax{ 1.05 };
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);
    h->GetYaxis()->SetTitle("good boards fraction");

    addChamberDelimiters(h, scaleMin, scaleMax);

    // draw horizontal limits
    TLine* l = new TLine(0, mMinGoodErrorFrac, h->GetXaxis()->GetXmax(), mMinGoodErrorFrac);
    l->SetLineColor(kBlue);
    l->SetLineStyle(kDashed);
    h->GetListOfFunctions()->Add(l);

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if (mo->getName().find("SyncedBoardsFractionPerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (!h) {
      return;
    }

    float scaleMin{ 0 };
    float scaleMax{ 1.05 };
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);

    addChamberDelimiters(h, scaleMin, scaleMax);

    // draw horizontal limits
    TLine* l = new TLine(0, mMinGoodSyncFrac, h->GetXaxis()->GetXmax(), mMinGoodSyncFrac);
    l->SetLineColor(kBlue);
    l->SetLineStyle(kDashed);
    h->GetListOfFunctions()->Add(l);

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::muonchambers
