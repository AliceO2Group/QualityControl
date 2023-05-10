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
#include <TStyle.h>
#include <TColor.h>
#include <TH1.h>
#include <TH2.h>
#include <TPaletteAxis.h>
#include <TList.h>
#include <TLatex.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::mid
{

void DigitsQcCheck::configure()
{ // default params ::
  mMeanMultThreshold = 100.;
  mMinMultThreshold = 0.;
  mOrbTF = 32;
  mLocalBoardScale = 100;     // kHz
  mLocalBoardThreshold = 400; // kHz
  mNbBadLocalBoard = 10;
  mNbEmptyLocalBoard = 117; // 234/2

  ILOG(Info, Devel) << "configure DigitsQcCheck" << ENDM;
  if (auto param = mCustomParameters.find("MeanMultThreshold"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MeanMultThreshold: " << param->second << ENDM;
    mMeanMultThreshold = stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinMultThreshold"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MinMultThreshold: " << param->second << ENDM;
    mMinMultThreshold = stof(param->second);
  }
  if (auto param = mCustomParameters.find("NbOrbitPerTF"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :NbOrbitPerTF " << param->second << ENDM;
    mOrbTF = stof(param->second);
  }
  if (auto param = mCustomParameters.find("LocalBoardScale"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :LocalBoardScale " << param->second << ENDM;
    mLocalBoardScale = stof(param->second);
  }
  if (auto param = mCustomParameters.find("LocalBoardThreshold"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :LocalBoardThreshold " << param->second << ENDM;
    mLocalBoardThreshold = stof(param->second);
  }
  if (auto param = mCustomParameters.find("NbBadLocalBoard"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :NbBadLocalBoard " << param->second << ENDM;
    mNbBadLocalBoard = stof(param->second);
  }
  if (auto param = mCustomParameters.find("NbEmptyLocalBoard"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :NbEmptyLocalBoard " << param->second << ENDM;
    mNbEmptyLocalBoard = stof(param->second);
  }
  // std::cout << " mLocalBoardThreshold  = "<<mLocalBoardThreshold << std::endl ;
  // std::cout << " mNbBadLocalBoard  = "<<mNbBadLocalBoard << std::endl ;
  mscale = 0;
}

Quality DigitsQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // ILOG(Info, Devel) << "check DigitsQcCheck" << ENDM;
  Quality result = Quality::Null;
  float mean = 0.;
  float midThreshold = mMeanMultThreshold / 2;

  for (auto& [moName, mo] : *moMap) {
    // Nb Time Frame ::
    (void)moName;
    if (mo->getName() == "NbDigitTF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mDigitTF = h->GetBinContent(1);
      // std::cout << " mDigitTF  = "<< mDigitTF << std::endl ;
    }

    (void)moName;
    // Bend Multiplicity Histo ::
    if (mo->getName() == "MultHitMT11B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT11 = Quality::Good;
      mean = h->GetMean();
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
        resultBMT11 = Quality::Bad;
      else if (mean > midThreshold)
        resultBMT11 = Quality::Medium;
      // std::cout << "check :: resultBMT11 =>>  " <<resultBMT11 << std::endl;
    } // end mMultHitMT11B check

    if (mo->getName() == "MultHitMT12B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT12 = Quality::Good;
      mean = h->GetMean();
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
        resultBMT12 = Quality::Bad;
      else if (mean > midThreshold)
        resultBMT12 = Quality::Medium;
    } // end mMultHitMT12B check

    if (mo->getName() == "MultHitMT21B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT21 = Quality::Good;
      mean = h->GetMean();
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
        resultBMT21 = Quality::Bad;
      else if (mean > midThreshold)
        resultBMT21 = Quality::Medium;
    } // end mMultHitMT21B check

    if (mo->getName() == "MultHitMT22B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultBMT22 = Quality::Good;
      mean = h->GetMean();
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
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
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
        resultNBMT11 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT11 = Quality::Medium;
    } // end mMultHitMT11NB check

    if (mo->getName() == "MultHitMT12NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT12 = Quality::Good;
      mean = h->GetMean();
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
        resultNBMT12 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT12 = Quality::Medium;
    } // end mMultHitMT12NB check

    if (mo->getName() == "MultHitMT21NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT21 = Quality::Good;
      mean = h->GetMean();
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
        resultNBMT21 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT21 = Quality::Medium;
    } // end mMultHitMT21NB check

    if (mo->getName() == "MultHitMT22NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      resultNBMT22 = Quality::Good;
      mean = h->GetMean();
      if ((mean > mMeanMultThreshold) || (mean < mMinMultThreshold))
        resultNBMT22 = Quality::Bad;
      else if (mean > midThreshold)
        resultNBMT22 = Quality::Medium;
    } // end mMultHitMT22NB check

    if (mo->getName() == "LocalBoardsMap") {
      mBadLB = 0;
      mEmptyLB = 0;
      if (mDigitTF > 0) {
        mscale = 1;
        float scale = 1 / (mDigitTF * scaleTime * mOrbTF * 1000); // (kHz)
        // std::cout << " scale = "<< scale << std::endl ;
        auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
        h2->Scale(scale);
        for (int bx = 1; bx < 15; bx++) {
          for (int by = 1; by < 37; by++) {
            if (!((bx > 6) && (bx < 9) && (by > 15) && (by < 22))) { // central zone empty
              if (!(h2->GetBinContent(bx, by)))
                mEmptyLB++;
              else if (h2->GetBinContent(bx, by) > mLocalBoardThreshold)
                mBadLB++;
              // std::cout <<"        mBad:"<<mBadLB<<" ; mEmpt:"<<mEmptyLB<<" :: Board( " << bx <<";"<< by <<") ==> "<< h2->GetBinContent(bx,by) <<std::endl;
              if ((bx == 1) || (bx == 14) || (by == 1) || (by == 33))
                by += 3; // zones 1 board
              else if (!((bx > 4) && (bx < 11) && (by > 12) && (by < 25)))
                by += 1; // zones 2 boards
            }
          }
        }
      }
    } // if mDigitTF>0
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
  tl->SetTextSize(0.08);
  tl->SetTextColor(color);

  return tl;
}

void DigitsQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // ILOG(Info, Devel) << "beautify DigitsQcCheck" << ENDM;
  gStyle->SetPalette(kRainBow);

  TLatex* msg;
  unsigned long mean = 0.;

  // Bend Multiplicity Histo ::
  if (mDigitTF > 5) {
    if (mo->getName() == "MultHitMT11B") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mean = h->GetMean();
      if (resultBMT11 == Quality::Good) {
        // std::cout << "beautify :: BMT11 mean =>>  " << mean << std::endl;
        h->SetFillColor(kGreen);
        msg = drawLatex(.15, 0.82, kGreen, Form("Limit : [%4.2f;%4.1f]", mMinMultThreshold, mMeanMultThreshold));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean=%4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT11 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.2, 0.72, kRed, Form("Limit : [%4.2f;%4.1f]", mMinMultThreshold, mMeanMultThreshold));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f  ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT11 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.2, 0.72, kOrange, Form("Limit : [%4.2f;%4.1f]", mMinMultThreshold, mMeanMultThreshold / 2));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
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
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT12 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f  ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT12 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
        h->GetListOfFunctions()->Add(msg);
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
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT21 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT21 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
        h->GetListOfFunctions()->Add(msg);
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
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean = %4.1f  ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT22 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT22 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
        h->GetListOfFunctions()->Add(msg);
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
        h->SetFillColor(kGreen);
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT11 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT11 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
        h->GetListOfFunctions()->Add(msg);
      }
      h->GetListOfFunctions()->Add(msg);
      h->SetTitleSize(0.04);
      h->SetLineColor(kBlack);
    }
    if (mo->getName() == "MultHitMT12NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mean = h->GetMean();
      if (resultNBMT12 == Quality::Good) {
        h->SetFillColor(kGreen);
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT12 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT12 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
        h->GetListOfFunctions()->Add(msg);
      }
      h->GetListOfFunctions()->Add(msg);
      h->SetTitleSize(0.04);
      h->SetLineColor(kBlack);
    }
    if (mo->getName() == "MultHitMT21NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mean = h->GetMean();
      if (resultNBMT21 == Quality::Good) {
        h->SetFillColor(kGreen);
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT21 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT21 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
        h->GetListOfFunctions()->Add(msg);
      }
      h->GetListOfFunctions()->Add(msg);
      h->SetTitleSize(0.04);
      h->SetLineColor(kBlack);
    }
    if (mo->getName() == "MultHitMT22NB") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mean = h->GetMean();
      if (resultNBMT22 == Quality::Good) {
        h->SetFillColor(kGreen);
        msg = drawLatex(.3, 0.62, kGreen, Form("Mean = %4.1f ", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kGreen, "Quality::Good");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT22 == Quality::Bad) {
        h->SetFillColor(kRed);
        msg = drawLatex(.3, 0.62, kRed, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kRed, "Quality::Bad ");
        h->GetListOfFunctions()->Add(msg);
      } else if (resultBMT22 == Quality::Medium) {
        h->SetFillColor(kOrange);
        msg = drawLatex(.3, 0.62, kOrange, Form("Mean = %4.1f", h->GetMean()));
        h->GetListOfFunctions()->Add(msg);
        msg = drawLatex(.3, 0.52, kOrange, "Quality::Medium ");
        h->GetListOfFunctions()->Add(msg);
      }
      h->GetListOfFunctions()->Add(msg);
      h->SetTitleSize(0.04);
      h->SetLineColor(kBlack);
    }

    /// Display Normalization (Hz)
    Double_t zcontoursLoc[18] = { 0, 0.05, 0.1, 0.5, 1, 5, 8, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 400 };
    Double_t zcontoursLoc4[18];
    Double_t zcontoursStrip[18] = { 0, 0.02, 0.05, 0.08, 0.1, 0.3, 0.5, 0.8, 1, 3, 5, 8, 10, 12, 14, 16, 18, 20 };
    for (int i = 0; i < 18; i++) {
      zcontoursLoc[i] = zcontoursLoc[i] / 400 * mLocalBoardScale;
      zcontoursLoc4[i] = zcontoursLoc[i] * 4;
      zcontoursStrip[i] = zcontoursLoc[i] / 10;
    }
    int ncontours = 18;

    /// Local Boards Display
    float scale = 1 / (mDigitTF * scaleTime * mOrbTF * 1000); // (kHz)
    if (mo->getName() == "LocalBoardsMap") {
      if (mscale) {
        auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
        // h2->GetZaxis()->SetTitleOffset(1.5);
        // h2->GetZaxis()->SetTitleSize(0.05);
        // h2->GetZaxis()->SetTitle("kHz");

        // std::cout << " scale = "<< scale << std::endl ;
        updateTitle(h2, "(kHz)");
        updateTitle(h2, Form("- TF=%3.0f -", mDigitTF));
        h2->SetMaximum(mLocalBoardScale);
        h2->SetContour(ncontours, zcontoursLoc4);

        if (mBadLB > mNbBadLocalBoard) {
          msg = drawLatex(.12, 0.72, kRed, Form(" %d boards > %3.1f kHz", mBadLB, mLocalBoardThreshold));
          h2->GetListOfFunctions()->Add(msg);
          msg = drawLatex(.3, 0.32, kRed, "Quality::BAD ");
          h2->GetListOfFunctions()->Add(msg);
          QualLocBoards = Quality::Bad;
        } else if (mBadLB >= 1) {
          msg = drawLatex(.12, 0.72, kOrange, Form(" %d boards > %3.1f kHz", mBadLB, mLocalBoardThreshold));
          h2->GetListOfFunctions()->Add(msg);
          msg = drawLatex(.25, 0.32, kOrange, "Quality::MEDIUM ");
          h2->GetListOfFunctions()->Add(msg);
          QualLocBoards = Quality::Medium;
        }
        if (mEmptyLB > mNbEmptyLocalBoard) {
          msg = drawLatex(.15, 0.62, kRed, Form(" %d boards empty", mEmptyLB));
          h2->GetListOfFunctions()->Add(msg);
          msg = drawLatex(.3, 0.32, kRed, "Quality::BAD ");
          h2->GetListOfFunctions()->Add(msg);
          QualLocBoards = Quality::Bad;
        } else if (mEmptyLB >= mNbEmptyLocalBoard / 3) {
          msg = drawLatex(.12, 0.62, kOrange, Form(" %d boards empty", mEmptyLB));
          h2->GetListOfFunctions()->Add(msg);
          msg = drawLatex(.25, 0.32, kOrange, "Quality::MEDIUM ");
          h2->GetListOfFunctions()->Add(msg);
          QualLocBoards = Quality::Medium;
        } else if (mBadLB == 0) {
          msg = drawLatex(.3, 0.32, kGreen, "Quality::GOOD ");
          h2->GetListOfFunctions()->Add(msg);
          QualLocBoards = Quality::Good;
        }
      }
    }
    if (mo->getName() == "LocalBoardsMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursLoc);
      h2->SetMaximum(mLocalBoardScale / 4);
    }
    if (mo->getName() == "LocalBoardsMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursLoc);
      h2->SetMaximum(mLocalBoardScale / 4);
    }
    if (mo->getName() == "LocalBoardsMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursLoc);
      h2->SetMaximum(mLocalBoardScale / 4);
    }
    if (mo->getName() == "LocalBoardsMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursLoc);
      h2->SetMaximum(mLocalBoardScale / 4);
    }
    /// Strips Display
    if (mo->getName() == "BendHitsMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    if (mo->getName() == "BendHitsMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    if (mo->getName() == "BendHitsMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    if (mo->getName() == "BendHitsMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    if (mo->getName() == "NBendHitsMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    if (mo->getName() == "NBendHitsMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    if (mo->getName() == "NBendHitsMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    if (mo->getName() == "NBendHitsMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, "(kHz)");
      updateTitle(h2, Form("TF= %3.0f", mDigitTF));
      h2->Scale(scale);
      h2->SetContour(ncontours, zcontoursStrip);
      h2->SetMaximum(mLocalBoardScale / 20);
    }
    gStyle->SetPalette(kRainBow);
    auto currentTime = getCurrentTime();
    updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);
  }
}

} // namespace o2::quality_control_modules::mid
