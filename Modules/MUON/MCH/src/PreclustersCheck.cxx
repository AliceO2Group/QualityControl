// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PreclustersCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCH/PreclustersCheck.h"
#include "MCH/Helpers.h"
#include "MUONCommon/Helpers.h"
#include "QualityControl/ReferenceUtils.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"

// ROOT
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TList.h>
#include <TLine.h>
#include <TPaveLabel.h>
#include <fstream>
#include <string>

using namespace std;
using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

void PreclustersCheck::configure()
{
}

void PreclustersCheck::startOfActivity(const Activity& activity)
{
  mMeanEffHistNameB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffHistNameB", mMeanEffHistNameB, activity);
  mMeanEffHistNameNB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffHistNameNB", mMeanEffHistNameNB, activity);
  mMeanEffPerSolarHistName = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffPerSolarHistName", mMeanEffPerSolarHistName, activity);

  mMeanEffRefCompHistNameB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffRefCompHistNameB", mMeanEffRefCompHistNameB, activity);
  mMeanEffRefCompHistNameNB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffRefCompHistNameNB", mMeanEffRefCompHistNameNB, activity);
  mMeanEffPerSolarRefCompHistName = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffPerSolarRefCompHistName", mMeanEffPerSolarRefCompHistName, activity);

  getThresholdsPerStation(mCustomParameters, activity, "MinEfficiency", mMinEfficiencyPerStation, mMinEfficiency);
  mMinEfficiencyPerSolar = getConfigurationParameter<double>(mCustomParameters, "MinEfficiencyPerSolar", mMinEfficiencyPerSolar, activity);

  mMinEfficiencyRatio = getConfigurationParameter<double>(mCustomParameters, "MinEfficiencyRatio", mMinEfficiencyRatio, activity);
  mMinEfficiencyRatioPerSolar = getConfigurationParameter<double>(mCustomParameters, "MinEfficiencyRatioPerSolar", mMinEfficiencyRatioPerSolar, activity);

  mPseudoeffPlotScaleMin = getConfigurationParameter<double>(mCustomParameters, "PseudoeffPlotScaleMin", mPseudoeffPlotScaleMin, activity);
  mPseudoeffPlotScaleMax = getConfigurationParameter<double>(mCustomParameters, "PseudoeffPlotScaleMax", mPseudoeffPlotScaleMax, activity);

  mEfficiencyRatioScaleRange = getConfigurationParameter<double>(mCustomParameters, "EfficiencyRatioScaleRange", mEfficiencyRatioScaleRange, activity);
  mEfficiencyRatioPerSolarScaleRange = getConfigurationParameter<double>(mCustomParameters, "EfficiencyRatioPerSolarScaleRange", mEfficiencyRatioPerSolarScaleRange, activity);

  mMaxBadST12 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST12", mMaxBadST12, activity);
  mMaxBadST345 = getConfigurationParameter<int>(mCustomParameters, "MaxBadDE_ST345", mMaxBadST345, activity);

  mQualityChecker.mMaxBadST12 = mMaxBadST12;
  mQualityChecker.mMaxBadST345 = mMaxBadST345;
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

template <typename Lambda>
std::array<Quality, getNumDE()> checkPlot(TH1* h, Lambda check)
{
  std::array<Quality, getNumDE()> result;
  std::fill(result.begin(), result.end(), Quality::Null);

  if (h->GetEntries() == 0) {
    return result;
  }

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

std::array<Quality, getNumDE()> PreclustersCheck::checkMeanEfficiencies(TH1* h)
{
  auto checkFunction = [&](double val, int station) -> bool {
    auto minEfficiency = mMinEfficiency;
    if (station >= 0 && station < 5) {
      if (mMinEfficiencyPerStation[station]) {
        minEfficiency = mMinEfficiencyPerStation[station].value();
      }
    }
    return (val >= minEfficiency);
  };
  return checkPlot(h, checkFunction);
}

std::array<Quality, getNumDE()> PreclustersCheck::checkMeanEfficiencyRatios(TH1* h)
{
  auto checkFunction = [&](double val, int /*station*/) -> bool {
    auto minEfficiency = mMinEfficiencyRatio;
    return (val >= minEfficiency);
  };
  return checkPlot(h, checkFunction);
}

void PreclustersCheck::checkSolarMeanEfficiencies(TH1* h)
{
  for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
    int bin = solar + 1;
    double value = h->GetBinContent(bin);
    if (value >= mMinEfficiencyPerSolar) {
      continue;
    }
    mSolarQuality[solar] = Quality::Bad;
  }
}

void PreclustersCheck::checkSolarMeanEfficiencyRatios(TH1* h)
{
  for (int solar = 0; solar < h->GetXaxis()->GetNbins(); solar++) {
    int bin = solar + 1;
    double value = h->GetBinContent(bin);
    if (value >= mMinEfficiencyRatio) {
      continue;
    }
    mSolarQuality[solar] = Quality::Bad;
  }
}

Quality PreclustersCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  ILOG(Debug, Devel) << "Entered PreclustersCheck::check" << ENDM;
  ILOG(Debug, Devel) << "   received a list of size : " << moMap->size() << ENDM;
  for (auto& [moName, mo] : *moMap) {
    ILOG(Debug, Devel) << "Object: " << moName << " | " << mo->getName() << ENDM;
  }

  mQualityChecker.reset();
  std::fill(mSolarQuality.begin(), mSolarQuality.end(), Quality::Good);

  for (auto& [moName, mo] : *moMap) {

    if (matchHistName(moName, mMeanEffHistNameB) || matchHistName(moName, mMeanEffHistNameNB)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h) {
        auto q = checkMeanEfficiencies(h);
        mQualityChecker.addCheckResult(q);
      }
    }

    if (matchHistName(moName, mMeanEffPerSolarHistName)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h) {
        checkSolarMeanEfficiencies(h);
      }
    }

    // Ratios with reference
    if (matchHistName(moName, mMeanEffRefCompHistNameB) || matchHistName(moName, mMeanEffRefCompHistNameNB)) {
      TCanvas* canvas = dynamic_cast<TCanvas*>(mo->getObject());
      if (canvas) {
        auto ratioPlot = o2::quality_control::checker::getRatioPlotFromCanvas(canvas);
        if (ratioPlot) {
          auto q = checkMeanEfficiencyRatios(ratioPlot);
          mQualityChecker.addCheckResult(q);
        }
      }
    }

    if (matchHistName(moName, mMeanEffPerSolarRefCompHistName)) {
      TCanvas* canvas = dynamic_cast<TCanvas*>(mo->getObject());
      if (canvas) {
        auto ratioPlot = o2::quality_control::checker::getRatioPlotFromCanvas(canvas);
        if (ratioPlot) {
          ILOG(Debug, Devel) << "Checking eff ratio for SOLAR:" << ENDM;
          checkSolarMeanEfficiencyRatios(ratioPlot);
        }
      }
    }
  }

  return mQualityChecker.getQuality();
}

void PreclustersCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
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

    if (matchHistName(mo->getName(), mMeanEffRefCompHistNameB) ||
        matchHistName(mo->getName(), mMeanEffRefCompHistNameNB)) {
      ratioPlotRange = mEfficiencyRatioScaleRange;
      ratioThreshold = mMinEfficiencyRatio;
      plotScaleMin = mPseudoeffPlotScaleMin;
      plotScaleMax = mPseudoeffPlotScaleMax;
      isSolar = false;
    } else if (matchHistName(mo->getName(), mMeanEffPerSolarRefCompHistName)) {
      ratioPlotRange = mEfficiencyRatioPerSolarScaleRange;
      ratioThreshold = mMinEfficiencyRatioPerSolar;
      plotScaleMin = mPseudoeffPlotScaleMin;
      plotScaleMax = mPseudoeffPlotScaleMax;
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

  if ((mo->getName().find("ChargeMPV") != std::string::npos)) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }

    h->SetMinimum(0);
    h->SetMaximum(2000);
    addChamberDelimiters(h, h->GetMinimum(), h->GetMaximum());
    addDEBinLabels(h);
  }

  if ((mo->getName().find("MeanClusterSize") != std::string::npos)) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }

    h->SetMinimum(0);
    h->SetMaximum(20);
    addChamberDelimiters(h, h->GetMinimum(), h->GetMaximum());
    addDEBinLabels(h);
  }

  if ((mo->getName().find("MeanEfficiency") != std::string::npos) ||
      (mo->getName().find("PreclustersPerDE") != std::string::npos) ||
      (mo->getName().find("PreclustersSignalPerDE") != std::string::npos)) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }

    if ((mo->getName().find("MeanEfficiencyB") != std::string::npos) ||
        (mo->getName().find("MeanEfficiencyNB") != std::string::npos)) {
      h->SetMinimum(mPseudoeffPlotScaleMin);
      h->SetMaximum(1.2);
    } else {
      h->SetMinimum(0);
      h->SetMaximum(1.05 * h->GetMaximum());
    }

    if (mo->getName().find("MeanEfficiencyPerSolar") != std::string::npos) {
      addChamberDelimitersToSolarHistogram(h, h->GetMinimum(), h->GetMaximum());
      addChamberLabelsForSolar(h);
    } else {
      addChamberDelimiters(h, h->GetMinimum(), h->GetMaximum());
      addChamberLabelsForDE(h);
    }

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mMeanEffHistNameB) ||
        matchHistName(mo->getName(), mMeanEffHistNameNB) ||
        matchHistName(mo->getName(), mMeanEffPerSolarHistName)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      if (matchHistName(mo->getName(), mMeanEffPerSolarHistName)) {
        drawThreshold(h, mMinEfficiencyPerSolar);
      } else {
        drawThresholdsPerStation(h, mMinEfficiencyPerStation, mMinEfficiency);
      }
    }
  }

  if ((mo->getName().find("Pseudoeff_ST12") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_ST345") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_B_XY") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_NB_XY") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(mPseudoeffPlotScaleMin);
    h->SetMaximum(1);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
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
      TPaveLabel* label = new TPaveLabel(0.2, 0.85, 0.8, 0.92, text.c_str(), "blNDC");
      label->SetBorderSize(1);
      h->GetListOfFunctions()->Add(label);

      ILOG(Warning, Support) << "[PreclustersCheck] " << text << ENDM;
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
      TPaveLabel* label = new TPaveLabel(0.2, 0.85, 0.8, 0.92, badSolarList.c_str(), "blNDC");
      label->SetBorderSize(1);
      h->GetListOfFunctions()->Add(label);

      ILOG(Warning, Support) << "[PreclustersCheck] " << badSolarList << ENDM;
    }
  }
}

} // namespace o2::quality_control_modules::muonchambers
