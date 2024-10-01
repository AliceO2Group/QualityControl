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
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"

// ROOT
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TList.h>
#include <TLine.h>
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
  getThresholdsPerStation(mCustomParameters, activity, "MinEfficiency", mMinEfficiencyPerStation, mMinEfficiency);

  mPseudoeffPlotScaleMin = getConfigurationParameter<double>(mCustomParameters, "PseudoeffPlotScaleMin", mPseudoeffPlotScaleMin, activity);
  mPseudoeffPlotScaleMax = getConfigurationParameter<double>(mCustomParameters, "PseudoeffPlotScaleMax", mPseudoeffPlotScaleMax, activity);

  mMeanEffHistNameB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffHistNameB", mMeanEffHistNameB, activity);
  mMeanEffHistNameNB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffHistNameNB", mMeanEffHistNameNB, activity);

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

std::array<Quality, getNumDE()> PreclustersCheck::checkMeanEfficiencies(TH1F* h)
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

Quality PreclustersCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  ILOG(Debug, Devel) << "Entered PreclustersCheck::check" << ENDM;
  ILOG(Debug, Devel) << "   received a list of size : " << moMap->size() << ENDM;
  for (const auto& item : *moMap) {
    ILOG(Debug, Devel) << "Object: " << item.second->getName() << ENDM;
  }

  mQualityChecker.reset();

  for (auto& [moName, mo] : *moMap) {

    if (matchHistName(mo->getName(), mMeanEffHistNameB) || matchHistName(mo->getName(), mMeanEffHistNameNB)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h) {
        auto q = checkMeanEfficiencies(h);
        mQualityChecker.addCheckResult(q);
      }
    }
  }

  return mQualityChecker.getQuality();
}

std::string PreclustersCheck::getAcceptedType() { return "TH1"; }

void PreclustersCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
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

    addChamberDelimiters(h, h->GetMinimum(), h->GetMaximum());
    addDEBinLabels(h);

    // only the plot used for the check is beautified by changing the color
    // and adding the horizontal lines corresponding to the thresholds
    if (matchHistName(mo->getName(), mMeanEffHistNameB) || matchHistName(mo->getName(), mMeanEffHistNameNB)) {
      if (checkResult == Quality::Good) {
        h->SetFillColor(kGreen);
      } else if (checkResult == Quality::Bad) {
        h->SetFillColor(kRed);
      } else if (checkResult == Quality::Medium) {
        h->SetFillColor(kOrange);
      }
      h->SetLineColor(kBlack);

      drawThresholdsPerStation(h, mMinEfficiencyPerStation, mMinEfficiency);
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
