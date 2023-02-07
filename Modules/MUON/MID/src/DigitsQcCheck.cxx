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
/// \file   DigitsQcCheck.cxx
/// \author Valerie Ramillien
///

#include "MID/DigitsQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TList.h>
#include <TLatex.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::mid
{

void DigitsQcCheck::configure()
{
  ILOG(Debug, Support) << "configure DigitsQcCheck" << ENDM;
  if (auto param = mCustomParameters.find("MeanMultThreshold"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - MeanMultThreshold: " << param->second << ENDM;
    mMeanMultThreshold = stof(param->second);
  }
}

Quality DigitsQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // ILOG(Info, Support) << "check DigitsQcCheck" << ENDM;
  Quality result = Quality::Null;
  float mean = 0.;
  float midThreshold = mMeanMultThreshold / 2;
  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    // Bend Multiplicity Histo ::
    if (mo->getName() == "MultHitMT11B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT11 = Quality::Good;
      mean = h->GetMean();
      if (mean > mMeanMultThreshold)
        resultBMT11 = Quality::Bad;
      else if (mean > midThreshold)
        resultBMT11 = Quality::Medium;
      // std::cout << "check :: resultBMT11 =>>  " <<resultBMT11 << std::endl;
    } // end mMultHitMT11B check

    if (mo->getName() == "MultHitMT12B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT12 = Quality::Good;
      mean = h->GetMean();
      if (mean > mMeanMultThreshold)
        resultBMT12 = Quality::Bad;
      else if (mean > midThreshold)
        resultBMT12 = Quality::Medium;
    } // end mMultHitMT12B check

    if (mo->getName() == "MultHitMT21B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT21 = Quality::Good;
      mean = h->GetMean();
      if (mean > mMeanMultThreshold)
        resultBMT21 = Quality::Bad;
      else if (mean > midThreshold)
        resultBMT21 = Quality::Medium;
    } // end mMultHitMT21B check

    if (mo->getName() == "MultHitMT22B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT22 = Quality::Good;
      mean = h->GetMean();
      if (mean > mMeanMultThreshold)
        resultBMT22 = Quality::Bad;
      else if (mean > midThreshold)
        resultBMT22 = Quality::Medium;
    } // end mMultHitMT22B check

    // Non-Bend Multiplicity Histo ::
    if (mo->getName() == "MultHitMT11NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT11 = Quality::Good;
      mean = h->GetMean();
      // std::cout << "check :: NBMT11 mean =>>  " << mean << std::endl;
      if (mean > mMeanMultThreshold)
        resultNBMT11 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT11 = Quality::Medium;
    } // end mMultHitMT11NB check

    if (mo->getName() == "MultHitMT12NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT12 = Quality::Good;
      mean = h->GetMean();
      if (mean > mMeanMultThreshold)
        resultNBMT12 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT12 = Quality::Medium;
    } // end mMultHitMT12NB check

    if (mo->getName() == "MultHitMT21NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT21 = Quality::Good;
      mean = h->GetMean();
      if (mean > mMeanMultThreshold)
        resultNBMT21 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT21 = Quality::Medium;
    } // end mMultHitMT21NB check

    if (mo->getName() == "MultHitMT22NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT22 = Quality::Good;
      mean = h->GetMean();
      if (mean > mMeanMultThreshold)
        resultNBMT22 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT22 = Quality::Medium;
    } // end mMultHitMT22NB check
  }
  return result;
}

std::string DigitsQcCheck::getAcceptedType() { return "TH1"; }

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
  strftime(timestr, sizeof(timestr), "(%x - %X)", tmp);

  std::string result = timestr;
  return result;
}

static TLatex* drawLatex(double xmin, double ymin, Color_t color, TString text)
{

  TLatex* tl = new TLatex(xmin, ymin, Form("%s", text.Data()));
  tl->SetNDC();
  tl->SetTextFont(22); // Normal 42
  tl->SetTextSize(0.04);
  tl->SetTextColor(color);

  return tl;
}

void DigitsQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // ILOG(Info, Support) << "beautify DigitsQcCheck" << ENDM;

  TLatex* msg;
  unsigned long mean = 0.;
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  // Bend Multiplicity Histo ::
  if (mo->getName() == "MultHitMT11B") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultBMT11 == Quality::Good) {
      // std::cout << "beautify :: BMT11 mean =>>  " << mean << std::endl;
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
      h->GetListOfFunctions()->Add(msg);
    } else if (resultBMT11 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
      h->GetListOfFunctions()->Add(msg);
    } else if (resultBMT11 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
      h->GetListOfFunctions()->Add(msg);
    }
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "MultHitMT12B") {
    // std::cout << "beautify :: BMT12 =>>  " << resultBMT12 << std::endl;
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultBMT12 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
    } else if (resultBMT12 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
    } else if (resultBMT12 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
    }
    h->GetListOfFunctions()->Add(msg);
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "MultHitMT21B") {
    // std::cout << "beautify :: BMT21 =>>  " << resultBMT21 << std::endl;
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultBMT21 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
    } else if (resultBMT21 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
    } else if (resultBMT21 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
    }
    h->GetListOfFunctions()->Add(msg);
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "MultHitMT22B") {
    // std::cout << "beautify :: BMT22 =>>  " << resultBMT22 << std::endl;
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultBMT22 == Quality::Good) {
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
    } else if (resultBMT22 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
    } else if (resultBMT22 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
    }
    h->GetListOfFunctions()->Add(msg);
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
  // Non-Bend Multiplicity Histo ::
  if (mo->getName() == "MultHitMT11NB") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultNBMT11 == Quality::Good) {
      // std::cout << "beautify :: BMT11 mean =>>  " << mean << std::endl;
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
    } else if (resultNBMT11 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
    } else if (resultNBMT11 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
    }
    h->GetListOfFunctions()->Add(msg);
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "MultHitMT12NB") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultNBMT12 == Quality::Good) {
      // std::cout << "beautify :: BMT11 mean =>>  " << mean << std::endl;
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
    } else if (resultNBMT12 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
    } else if (resultNBMT12 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
    }
    h->GetListOfFunctions()->Add(msg);
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "MultHitMT21NB") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultNBMT21 == Quality::Good) {
      // std::cout << "beautify :: BMT11 mean =>>  " << mean << std::endl;
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
    } else if (resultNBMT21 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
    } else if (resultNBMT21 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
    }
    h->GetListOfFunctions()->Add(msg);
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
  if (mo->getName() == "MultHitMT22NB") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    mean = h->GetMean();
    if (resultNBMT22 == Quality::Good) {
      // std::cout << "beautify :: BMT11 mean =>>  " << mean << std::endl;
      h->SetFillColor(kGreen);
      msg = drawLatex(.15, 0.72, kGreen, Form("Mean value <%4.1f  Quality::Good", mMeanMultThreshold));
    } else if (resultNBMT22 == Quality::Bad) {
      h->SetFillColor(kRed);
      msg = drawLatex(.15, 0.72, kRed, Form("Mean value >%4.1f  Quality::Bad ", mMeanMultThreshold));
    } else if (resultNBMT22 == Quality::Medium) {
      h->SetFillColor(kOrange);
      msg = drawLatex(.15, 0.72, kOrange, Form("Mean value >%4.1f Quality::Medium ", mMeanMultThreshold / 2));
    }
    h->GetListOfFunctions()->Add(msg);
    h->SetTitleSize(0.04);
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::mid
