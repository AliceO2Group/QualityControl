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
#include "DataFormatsQualityControl/FlagReasons.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TStyle.h>
#include <TColor.h>
#include <TH1.h>
#include <TH2.h>

namespace o2::quality_control_modules::mid
{

void DigitsQcCheck::configure()
{
  ILOG(Info, Devel) << "configure DigitsQcCheck" << ENDM;
  if (auto param = mCustomParameters.find("MeanMultThreshold"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MeanMultThreshold: " << param->second << ENDM;
    mMeanMultThreshold = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinMultThreshold"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - MinMultThreshold: " << param->second << ENDM;
    mMinMultThreshold = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("NbOrbitPerTF"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - NbOrbitPerTF: " << param->second << ENDM;
    mHistoHelper.setNOrbitsPerTF(std::stol(param->second));
  }
  if (auto param = mCustomParameters.find("LocalBoardScale"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - LocalBoardScale: " << param->second << ENDM;
    mLocalBoardScale = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("LocalBoardThreshold"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - LocalBoardThreshold: " << param->second << ENDM;
    mLocalBoardThreshold = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("NbBadLocalBoard"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - NbBadLocalBoard: " << param->second << ENDM;
    mNbBadLocalBoard = std::stoi(param->second);
  }
  if (auto param = mCustomParameters.find("NbEmptyLocalBoard"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - NbEmptyLocalBoard: " << param->second << ENDM;
    mNbEmptyLocalBoard = std::stoi(param->second);
  }
}

Quality DigitsQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  // This info must be available from the beginning
  TH1* meanMultiHits = nullptr;
  for (auto& item : *moMap) {
    if (item.second->getName() == "NbDigitTF") {
      mHistoHelper.setNTFs(static_cast<TH1F*>(item.second->getObject())->GetBinContent(1));
    } else if (item.second->getName() == "MeanMultiHits") {
      meanMultiHits = static_cast<TH1*>(item.second->getObject());
    }
  }

  // Fill the summary multiplicity histogram
  if (meanMultiHits) {
    meanMultiHits->Reset();
    std::unordered_map<std::string, int> ref;
    for (int ibin = 1; ibin <= meanMultiHits->GetNbinsX(); ++ibin) {
      std::string hName = "MultHit";
      hName += meanMultiHits->GetXaxis()->GetBinLabel(ibin);
      ref[hName] = ibin;
    }

    int nGood = 0, nNull = 0, nBad = 0, nMedium = 0;
    auto globalQual = Quality::Null;
    for (auto& item : *moMap) {
      if (item.second->getName().find("MultHitMT") != std::string::npos) {
        std::string hName = item.second->getName();
        auto mean = static_cast<TH1*>(item.second->getObject())->GetMean();
        meanMultiHits->SetBinContent(ref[hName], mean);
        auto qual = Quality::Good;
        if (mean == 0.) {
          ++nNull;
          qual = Quality::Null;
        } else if (mean > mMeanMultThreshold || mean < mMinMultThreshold) {
          qual = Quality::Bad;
          result = qual;
          ++nBad;
        } else if (mean > mMeanMultThreshold / 2.) {
          qual = Quality::Medium;
          ++nMedium;
        } else {
          ++nGood;
        }
        mQualityMap[hName] = qual;
      }
    }
    if (nBad > 0) {
      globalQual = Quality::Bad;
    } else if (nMedium > 0) {
      globalQual = Quality::Medium;
    } else if (nGood == 8) {
      globalQual = Quality::Good;
    }
    mQualityMap[meanMultiHits->GetName()] = globalQual;
  }

  for (auto& item : *moMap) {
    if (item.second->getName() == "LocalBoardsMap") {
      if (mHistoHelper.getNTFs() > 0) {
        int nEmptyLB = 0;
        int nBadLB = 0;
        auto histo = static_cast<TH2F*>(item.second->getObject());
        mHistoHelper.normalizeHistoTokHz(histo);
        for (int bx = 1; bx < 15; bx++) {
          for (int by = 1; by < 37; by++) {
            if (!((bx > 6) && (bx < 9) && (by > 15) && (by < 22))) { // central zone empty
              double val = histo->GetBinContent(bx, by);
              if (val == 0) {
                nEmptyLB++;
              } else if (val > mLocalBoardThreshold) {
                nBadLB++;
              }
              if ((bx == 1) || (bx == 14) || (by == 1) || (by == 33))
                by += 3; // zones 1 board
              else if (!((bx > 4) && (bx < 11) && (by > 12) && (by < 25)))
                by += 1; // zones 2 boards
            }
          }
        }
        auto qual = Quality::Good;
        if (nBadLB > 0) {
          qual = Quality::Medium;
          if (nBadLB > mNbBadLocalBoard) {
            qual = Quality::Bad;
          }
          auto flag = o2::quality_control::FlagReason();
          qual.addReason(flag, fmt::format("{} boards > {} kHz", nBadLB, mLocalBoardThreshold));
        } else if (nEmptyLB > 0) {
          qual = Quality::Medium;
          if (nEmptyLB > mNbEmptyLocalBoard) {
            qual = Quality::Bad;
          }
          auto flag = o2::quality_control::FlagReason();
          qual.addReason(flag, fmt::format("{} boards empty", nEmptyLB));
        }
        mQualityMap[item.second->getName()] = qual;
      } // if mNTFInSeconds > 0.
    }
  }
  return result;
}

std::string DigitsQcCheck::getAcceptedType() { return "TH1"; }

void DigitsQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  gStyle->SetPalette(kRainBow);

  auto found = mQualityMap.find(mo->getName());
  if (found != mQualityMap.end()) {
    checkResult = found->second;
  }

  auto color = mHistoHelper.getColor(checkResult);

  // Multiplicity Histograms
  if (mo->getName().find("MultHitMT") != std::string::npos) {
    // This matches "MultHitMT*"
    if (mHistoHelper.getNTFs() > 0) {
      auto histo = static_cast<TH1F*>(mo->getObject());
      histo->SetFillColor(color);
      mHistoHelper.addLatex(histo, 0.15, 0.82, color, Form("Limit : [%g;%g]", mMinMultThreshold, mMeanMultThreshold));
      mHistoHelper.addLatex(histo, 0.3, 0.62, color, Form("Mean=%g ", histo->GetMean()));
      mHistoHelper.addLatex(histo, 0.3, 0.52, color, fmt::format("Quality::{}", checkResult.getName()));
      histo->SetTitleSize(0.04);
      histo->SetLineColor(kBlack);
    }
  } else if (mo->getName() == "MeanMultiHits") {
    auto histo = static_cast<TH1F*>(mo->getObject());
    mHistoHelper.addLatex(histo, 0.3, 0.52, color, fmt::format("Quality::{}", checkResult.getName()));
    mHistoHelper.updateTitleWithNTF(histo);
    histo->SetStats(0);
  } else {
    // Change palette contours so that visibility of low-multiplicity strips or boards is improved
    std::vector<double> zcontoursLoc{ 0, 0.025, 0.05, 0.075, 0.1, 0.25, 0.5, 0.75, 1, 2.5, 5, 7.5, 10, 25, 50, 75, 100 };
    std::vector<double> zcontoursLoc4, zcontoursStrip;
    for (auto& con : zcontoursLoc) {
      con *= mLocalBoardScale / zcontoursLoc.back();
      zcontoursLoc4.emplace_back(con * 4);
      zcontoursStrip.emplace_back(con / 10.);
    }

    // Local Boards Display
    std::string lbHistoName = "LocalBoardsMap";
    if (mo->getName().find(lbHistoName) != std::string::npos) {
      // This matches "LocalBoardsMap*"
      auto histo = static_cast<TH2F*>(mo->getObject());
      if (mo->getName() == lbHistoName) {
        // This is LocalBoardsMap and it was already scaled in the checker
        if (!checkResult.getReasons().empty()) {
          mHistoHelper.addLatex(histo, 0.12, 0.72, color, checkResult.getReasons().front().second.c_str());
        }
        mHistoHelper.addLatex(histo, 0.3, 0.32, color, fmt::format("Quality::{}", checkResult.getName()));
        histo->SetMaximum(zcontoursLoc4.back());
        histo->SetContour(zcontoursLoc4.size(), zcontoursLoc4.data());
      } else {
        mHistoHelper.normalizeHistoTokHz(histo);
        histo->SetMaximum(zcontoursLoc.back());
        histo->SetContour(zcontoursLoc.size(), zcontoursLoc.data());
      }
      mHistoHelper.updateTitleWithNTF(histo);
      histo->SetStats(0);
    } else {

      // Strips Display
      if (mo->getName().find("BendHitsMap") != std::string::npos) {
        // This matches both [N]BendHitsMap*
        int maxStrip = 20; // 20kHz Max Display
        auto histo = static_cast<TH2F*>(mo->getObject());
        mHistoHelper.normalizeHistoTokHz(histo);
        histo->SetMaximum(zcontoursStrip.back());
        histo->SetContour(zcontoursStrip.size(), zcontoursStrip.data());
        histo->SetStats(0);
      } else if (mo->getName() == "Hits") {
        auto histo = static_cast<TH1F*>(mo->getObject());
        mHistoHelper.normalizeHistoTokHz(histo);
        histo->SetStats(0);
      }
    }
  }
  mHistoHelper.updateTitle(dynamic_cast<TH1*>(mo->getObject()), mHistoHelper.getCurrentTime());
}
} // namespace o2::quality_control_modules::mid
