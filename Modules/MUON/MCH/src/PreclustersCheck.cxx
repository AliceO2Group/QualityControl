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
  mMinEfficiency = getConfigurationParameter<double>(mCustomParameters, "MinEfficiency", mMinEfficiency, activity);
  mMaxEffDelta = getConfigurationParameter<double>(mCustomParameters, "MaxEfficiencyDelta", mMaxEffDelta, activity);
  mPseudoeffPlotScaleMin = getConfigurationParameter<double>(mCustomParameters, "PseudoeffPlotScaleMin", mPseudoeffPlotScaleMin, activity);
  mPseudoeffPlotScaleMax = getConfigurationParameter<double>(mCustomParameters, "PseudoeffPlotScaleMax", mPseudoeffPlotScaleMax, activity);

  mMeanEffHistNameB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffHistNameB", mMeanEffHistNameB, activity);
  mMeanEffHistNameNB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffHistNameNB", mMeanEffHistNameNB, activity);
  mMeanEffRatioHistNameB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffRatioHistNameB", mMeanEffRatioHistNameB, activity);
  mMeanEffRatioHistNameNB = getConfigurationParameter<std::string>(mCustomParameters, "MeanEffRatioHistNameNB", mMeanEffRatioHistNameNB, activity);

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

std::array<Quality, getNumDE()> PreclustersCheck::checkMeanEfficiencies(TH1F* h)
{
  return checkPlot(h, [&](double val) -> bool { return (val >= mMinEfficiency); });
}

std::array<Quality, getNumDE()> PreclustersCheck::checkMeanEfficienciesRatio(TH1F* h)
{
  return checkPlot(h, [&](double val) -> bool { return (std::abs(val - 1) <= mMaxEffDelta); });
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

    if (matchHistName(mo->getName(), mMeanEffRatioHistNameB) || matchHistName(mo->getName(), mMeanEffRatioHistNameNB)) {
      TH1F* h = getHisto<TH1F>(mo);
      if (h && h->GetEntries() > 0) {
        auto q = checkMeanEfficienciesRatio(h);
        mQualityChecker.addCheckResult(q);
      }
    }
  }

  return mQualityChecker.getQuality();
}

std::string PreclustersCheck::getAcceptedType() { return "TH1"; }

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

void PreclustersCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);
  updateTitle(dynamic_cast<TCanvas*>(mo->getObject()), currentTime);

  if ((mo->getName().find("ChargeMPV") != std::string::npos)) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }
    if ((mo->getName().find("ChargeMPVRefRatio") != std::string::npos)) {
      h->SetMinimum(0.5);
      h->SetMaximum(1.5);
    } else {
      h->SetMinimum(0);
      h->SetMaximum(2000);
    }
    addChamberDelimiters(h, h->GetMinimum(), h->GetMaximum());
  }

  if ((mo->getName().find("MeanClusterSize") != std::string::npos)) {
    TH1F* h = getHisto<TH1F>(mo);
    if (!h) {
      return;
    }
    if ((mo->getName().find("MeanClusterSizeRefRatio") != std::string::npos)) {
      h->SetMinimum(0.8);
      h->SetMaximum(1.2);
    } else {
      h->SetMinimum(0);
      h->SetMaximum(20);
    }
    addChamberDelimiters(h, h->GetMinimum(), h->GetMaximum());
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

      // draw horizontal limits
      TLine* l = new TLine(0, mMinEfficiency, h->GetXaxis()->GetXmax(), mMinEfficiency);
      l->SetLineColor(kBlue);
      l->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(l);
    } else if ((mo->getName().find("MeanEfficiencyRefRatio") != std::string::npos)) {
      h->SetMinimum(1.0 - mMaxEffDelta * 2.0);
      h->SetMaximum(1.0 + mMaxEffDelta * 2.0);

      // draw horizontal limits
      TLine* l = new TLine(0, 1, h->GetXaxis()->GetXmax(), 1);
      l->SetLineColor(kBlack);
      l->SetLineStyle(kDotted);
      h->GetListOfFunctions()->Add(l);

      if (h->GetEntries() > 0) {
        l = new TLine(0, 1.0 - mMaxEffDelta, h->GetXaxis()->GetXmax(), 1.0 - mMaxEffDelta);
        l->SetLineColor(kBlue);
        l->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(l);

        l = new TLine(0, 1.0 + mMaxEffDelta, h->GetXaxis()->GetXmax(), 1.0 + mMaxEffDelta);
        l->SetLineColor(kBlue);
        l->SetLineStyle(kDashed);
        h->GetListOfFunctions()->Add(l);
      }
    } else {
      h->SetMinimum(0);
      h->SetMaximum(1.05 * h->GetMaximum());
    }
    addChamberDelimiters(h, h->GetMinimum(), h->GetMaximum());

    if ((mo->getName().find("MeanEfficiencyB") != std::string::npos) ||
        (mo->getName().find("MeanEfficiencyNB") != std::string::npos) ||
        (mo->getName().find("MeanEfficiencyRefRatio") != std::string::npos)) {
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
