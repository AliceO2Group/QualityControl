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
#include "MUONCommon/Helpers.h"
#include "QualityControl/ReferenceUtils.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TLine.h>
#include <TPaveLabel.h>
#include <fstream>
#include <string>

using namespace std;
using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

void DecodingCheck::configure()
{
}

void DecodingCheck::startOfActivity(const Activity& activity)
{
  mGoodFracHistName = getConfigurationParameter<std::string>(mCustomParameters, "GoodFracHistName", mGoodFracHistName, activity);
  mSyncFracHistName = getConfigurationParameter<std::string>(mCustomParameters, "SyncFracHistName", mSyncFracHistName, activity);

  mGoodFracPerSolarHistName = getConfigurationParameter<std::string>(mCustomParameters, "GoodFracPerSolarHistName", mGoodFracPerSolarHistName, activity);
  mSyncFracPerSolarHistName = getConfigurationParameter<std::string>(mCustomParameters, "SyncFracPerSolarHistName", mSyncFracPerSolarHistName, activity);

  getThresholdsPerStation(mCustomParameters, activity, "MinGoodErrorFrac", mMinGoodErrorFracPerStation, mMinGoodErrorFrac);
  getThresholdsPerStation(mCustomParameters, activity, "MinGoodSyncFrac", mMinGoodSyncFracPerStation, mMinGoodSyncFrac);

  mMinGoodErrorFracPerSolar = getConfigurationParameter<double>(mCustomParameters, "MinGoodErrorFracPerSolar", mMinGoodErrorFracPerSolar, activity);
  mMinGoodSyncFracPerSolar = getConfigurationParameter<double>(mCustomParameters, "MinGoodSyncFracPerSolar", mMinGoodSyncFracPerSolar, activity);

  mMaxBadST12 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST12", mMaxBadST12, activity);
  mMaxBadST345 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST345", mMaxBadST345, activity);
  mQualityChecker.mMaxBadST12 = mMaxBadST12;
  mQualityChecker.mMaxBadST345 = mMaxBadST345;

  mMinHeartBeatRate = getConfigurationParameter<double>(mCustomParameters, "MinHeartBeatRate", mMinHeartBeatRate, activity);
  mMaxHeartBeatRate = getConfigurationParameter<double>(mCustomParameters, "MaxHeartBeatRate", mMaxHeartBeatRate, activity);
}

Quality DecodingCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  std::array<Quality, getNumDE()> errorsQuality;
  std::fill(errorsQuality.begin(), errorsQuality.end(), Quality::Null);
  std::array<Quality, getNumDE()> syncQuality;
  std::fill(syncQuality.begin(), syncQuality.end(), Quality::Null);

  mQualityChecker.reset();
  std::fill(mSolarQuality.begin(), mSolarQuality.end(), Quality::Good);

  for (auto& [moName, mo] : *moMap) {

    if (matchHistName(mo->getName(), mGoodFracHistName)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      for (auto de : o2::mch::constants::deIdsForAllMCH) {
        int chamberId = (de - 100) / 100;
        int stationId = chamberId / 2;

        int deId = getDEindex(de);
        if (deId < 0) {
          continue;
        }

        auto minGoodErrorFrac = mMinGoodErrorFrac;
        if (stationId >= 0 && stationId < 5) {
          if (mMinGoodErrorFracPerStation[stationId]) {
            minGoodErrorFrac = mMinGoodErrorFracPerStation[stationId].value();
          }
        }

        double val = h->GetBinContent(deId + 1);
        if (val < minGoodErrorFrac) {
          errorsQuality[deId] = Quality::Bad;
        } else {
          errorsQuality[deId] = Quality::Good;
        }
      }
      mQualityChecker.addCheckResult(errorsQuality);
    }

    if (matchHistName(mo->getName(), mGoodFracPerSolarHistName)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
        int bin = solar + 1;
        double value = h->GetBinContent(bin);
        if (value >= mMinGoodErrorFracPerSolar) {
          continue;
        }
        mSolarQuality[solar] = Quality::Bad;
      }
    }

    if (matchHistName(mo->getName(), mSyncFracHistName)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      for (auto de : o2::mch::constants::deIdsForAllMCH) {
        int chamberId = (de - 100) / 100;
        int stationId = chamberId / 2;

        int deId = getDEindex(de);
        if (deId < 0) {
          continue;
        }

        auto minGoodSyncFrac = mMinGoodSyncFrac;
        if (stationId >= 0 && stationId < 5) {
          if (mMinGoodSyncFracPerStation[stationId]) {
            minGoodSyncFrac = mMinGoodSyncFracPerStation[stationId].value();
          }
        }

        double val = h->GetBinContent(deId + 1);
        if (val < minGoodSyncFrac) {
          syncQuality[deId] = Quality::Bad;
        } else {
          syncQuality[deId] = Quality::Good;
        }
      }
      mQualityChecker.addCheckResult(syncQuality);
    }

    if (matchHistName(mo->getName(), mSyncFracPerSolarHistName)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
        int bin = solar + 1;
        double value = h->GetBinContent(bin);
        if (value >= mMinGoodSyncFracPerSolar) {
          continue;
        }
        mSolarQuality[solar] = Quality::Bad;
      }
    }
  }
  return mQualityChecker.getQuality();
}



void DecodingCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
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
    bool isSolar{ false };

    if (matchHistName(mo->getName(), mGoodFracRefCompHistName)) {
      ratioPlotRange = mGoodFracRatioPlotRange;
      ratioThreshold = mMinGoodErrorFracRatio;
      isSolar = false;
    } else if (matchHistName(mo->getName(), mGoodFracPerSolarRefCompHistName)) {
      ratioPlotRange = mGoodFracRatioPerSolarPlotRange;
      ratioThreshold = mMinGoodErrorFracRatioPerSolar;
      isSolar = true;
    } else if (matchHistName(mo->getName(), mSyncFracRefCompHistName)) {
      ratioPlotRange = mSyncFracRatioPlotRange;
      ratioThreshold = mMinGoodSyncFracRatio;
      isSolar = false;
    } else if (matchHistName(mo->getName(), mSyncFracPerSolarRefCompHistName)) {
      ratioPlotRange = mSyncFracRatioPerSolarPlotRange;
      ratioThreshold = mMinGoodSyncFracRatioPerSolar;
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
      refCompPlots.first->SetMinimum(0);
      refCompPlots.first->SetMaximum(1.05);
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
    addChamberLabelsForDE(h);

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mGoodFracHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThresholdsPerStation(h, mMinGoodErrorFracPerStation, mMinGoodErrorFrac);
    }
  }

  if (mo->getName().find("GoodBoardsFractionPerSolar") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (!h) {
      return;
    }

    float scaleMin{ 0 };
    float scaleMax{ 1.05 };
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);
    h->GetYaxis()->SetTitle("good boards fraction");

    addChamberDelimitersToSolarHistogram(h, scaleMin, scaleMax);
    addChamberLabelsForSolar(h);

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mGoodFracPerSolarHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThreshold(h, mMinGoodErrorFracPerSolar);
    }
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
    addChamberLabelsForDE(h);

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mSyncFracHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThresholdsPerStation(h, mMinGoodSyncFracPerStation, mMinGoodSyncFrac);
    }
  }

  if (mo->getName().find("SyncedBoardsFractionPerSolar") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (!h) {
      return;
    }

    float scaleMin{ 0 };
    float scaleMax{ 1.05 };
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);

    addChamberDelimitersToSolarHistogram(h, scaleMin, scaleMax);
    addChamberLabelsForSolar(h);

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mSyncFracPerSolarHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThreshold(h, mMinGoodSyncFracPerSolar);
    }
  }

  // Normalize the heartBeat rate plots
  if (mo->getName().find("HBRate_ST") != std::string::npos) {
    TH2F* h = dynamic_cast<TH2F*>(mo->getObject());
    if (!h) {
      return;
    }
    h->SetMinimum(mMinHeartBeatRate);
    h->SetMaximum(mMaxHeartBeatRate);
  }

  // update quality flags for each DE
  if (mo->getName().find("QualityFlagPerDE") != std::string::npos) {
    TH2F* h = dynamic_cast<TH2F*>(mo->getObject());
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

      if (!badDEs.empty()) {
        std::string text = std::string("Bad DEs: ") + badDEs;
        TPaveLabel* label = new TPaveLabel(0.2, 0.8, 0.8, 0.88, text.c_str(), "blNDC");
        label->SetBorderSize(1);
        h->GetListOfFunctions()->Add(label);

        ILOG(Warning, Support) << "[DecodingCheck] " << text << ENDM;
      }
    }
  }

  // update quality flags for each SOLAR
  if (mo->getName().find("QualityFlagPerSolar") != std::string::npos) {
    TH2F* h = dynamic_cast<TH2F*>(mo->getObject());
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

      ILOG(Warning, Support) << "[DecodingCheck] " << badSolarList << ENDM;
    }
  }
}

} // namespace o2::quality_control_modules::muonchambers
