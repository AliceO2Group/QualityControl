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
/// \file   HmpidRawChecks.cxx
/// \author Nicola Nicassio
///

#include "HMPID/HmpidRawChecks.h"

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TCanvas.h>
#include <TList.h>
#include <TMath.h>
#include <TLine.h>
#include <TPaveText.h>
#include <iostream>
#include <fstream>
#include <string>
#include <TIterator.h>

using namespace std;

namespace o2::quality_control_modules::hmpid
{
void HmpidRawChecks::configure()
{

  // Histo names
  m_hHmpBigMap_HistName = mCustomParameters.atOrDefaultValue("m_hHmpBigMap_HistName");
  m_hHmpHvSectorQ_HistName = mCustomParameters.atOrDefaultValue("m_hHmpHvSectorQ_HistName");
  m_hHmpPadOccPrf_HistName = mCustomParameters.atOrDefaultValue("m_hHmpPadOccPrf_HistName");
  m_hBusyTime_HistName = mCustomParameters.atOrDefaultValue("m_hBusyTime_HistName");
  m_hEventSize_HistName = mCustomParameters.atOrDefaultValue("m_hEventSize_HistName");
  m_hCheckHV_HistName = mCustomParameters.atOrDefaultValue("m_hCheckHV_HistName");

  // Histo limits
  mMinOccupancy = std::stof(mCustomParameters.atOrDefaultValue("mMinOccupancy"));
  mMaxOccupancy = std::stof(mCustomParameters.atOrDefaultValue("mMaxOccupancy"));
  mMinEventSize = std::stof(mCustomParameters.atOrDefaultValue("mMinEventSize"));
  mMaxEventSize = std::stof(mCustomParameters.atOrDefaultValue("mMaxEventSize"));
  mMinBusyTime = std::stof(mCustomParameters.atOrDefaultValue("mMinBusyTime"));
  mMaxBusyTime = std::stof(mCustomParameters.atOrDefaultValue("mMaxBusyTime"));
  mMinHVTotalEntriesToCheckQuality = std::stof(mCustomParameters.atOrDefaultValue("mMinHVTotalEntriesToCheckQuality"));
  mFractionXBinsHVSingleModuleEntriesToLabelGoodBadQuality = std::stof(mCustomParameters.atOrDefaultValue("mFractionXBinsHVSingleModuleEntriesToLabelGoodBadQuality"));
  mMaxBadDDLForMedium = std::stof(mCustomParameters.atOrDefaultValue("mMaxBadDDLForMedium"));
  mMaxBadDDLForBad = std::stof(mCustomParameters.atOrDefaultValue("mMaxBadDDLForBad"));
  mMaxBadHVForMedium = std::stof(mCustomParameters.atOrDefaultValue("mMaxBadHVForMedium"));
  mMaxBadHVForBad = std::stof(mCustomParameters.atOrDefaultValue("mMaxBadHVForBad"));
}

template <typename Lambda>
std::array<Quality, 14> checkPlot(TProfile* h, Lambda check)
{
  std::array<Quality, 14> result; // size == number of links
  std::fill(result.begin(), result.end(), Quality::Null);
  for (int eqId = 0; eqId < 14; eqId++) // <-- Entries in histogram are already decoded in eqId
  {
    int bin = eqId + 1;
    double val = h->GetBinContent(bin);
    if (val == 0) {
      result[eqId] = Quality::Null;
    } else if (check(val)) {
      result[eqId] = Quality::Good;
    } else {
      result[eqId] = Quality::Bad;
    }
  }
  return result;
}

std::array<Quality, 14> HmpidRawChecks::check_hHmpPadOccPrf(TProfile* h)
{
  return checkPlot(h, [&](double val) -> bool { return (val >= mMinOccupancy && val <= mMaxOccupancy); });
}

std::array<Quality, 14> HmpidRawChecks::check_hBusyTime(TProfile* h)
{
  return checkPlot(h, [&](double val) -> bool { return (val >= mMinBusyTime && val <= mMaxBusyTime); });
}

std::array<Quality, 14> HmpidRawChecks::check_hEventSize(TProfile* h)
{
  return checkPlot(h, [&](double val) -> bool { return (val >= mMinEventSize && val <= mMaxEventSize); });
}

template <typename Lambda>
std::array<Quality, 42> checkPlotHV(TH2F* h, Lambda check, double mMinHVTotalEntriesToCheckQuality)
{
  std::array<Quality, 42> result; // size == number of HV sectors
  std::fill(result.begin(), result.end(), Quality::Null);
  for (int HVId = 0; HVId < 42; HVId++) // <-- Entries in histogram are already decoded in eqId
  {
    int binY = HVId + 1;
    double val = 0;
    for (int binX = 1; binX <= h->GetNbinsX(); binX++) {
      if (h->GetBinContent(binX, binY) != 0) {
        val++;
      }
    }
    if (h->GetEntries() < mMinHVTotalEntriesToCheckQuality) {
      result[HVId] = Quality::Null;
    } else if (check(val)) {
      result[HVId] = Quality::Good;
    } else {
      result[HVId] = Quality::Bad;
    }
  }
  return result;
}

std::array<Quality, 42> HmpidRawChecks::check_hHmpHvSectorQ(TH2F* h) // <-- non 14 per 2D
{
  return checkPlotHV(
    h, [&](double val) -> bool { return (val >= mFractionXBinsHVSingleModuleEntriesToLabelGoodBadQuality * h->GetNbinsX()); }, mMinHVTotalEntriesToCheckQuality);
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
  if (obj->InheritsFrom("TProfile") || obj->InheritsFrom("TH2F") || obj->InheritsFrom("TProfile2D")) {
    h = dynamic_cast<T*>(obj);
  }

  if (obj->InheritsFrom("TCanvas")) {
    TCanvas* c = dynamic_cast<TCanvas*>(obj);
    h = getHisto<T>(c, mo->getName() + "Hist");
  }

  return h;
}

Quality HmpidRawChecks::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  mQualityOccupancy = Quality::Null;
  std::array<Quality, 14> qualityOccupancy;
  QualityCheckerDDL qualityCheckerOccupancy;
  qualityCheckerOccupancy.resetDDL();
  qualityCheckerOccupancy.mMaxBadDDLForMedium = mMaxBadDDLForMedium;
  qualityCheckerOccupancy.mMaxBadDDLForBad = mMaxBadDDLForBad;

  mQualityBusyTime = Quality::Null;
  std::array<Quality, 14> qualityBusyTime;
  QualityCheckerDDL qualityCheckerBusyTime;
  qualityCheckerBusyTime.resetDDL();
  qualityCheckerBusyTime.mMaxBadDDLForMedium = mMaxBadDDLForMedium;
  qualityCheckerBusyTime.mMaxBadDDLForBad = mMaxBadDDLForBad;

  mQualityEventSize = Quality::Null;
  std::array<Quality, 14> qualityEventSize;
  QualityCheckerDDL qualityCheckerEventSize;
  qualityCheckerEventSize.resetDDL();
  qualityCheckerEventSize.mMaxBadDDLForMedium = mMaxBadDDLForMedium;
  qualityCheckerEventSize.mMaxBadDDLForBad = mMaxBadDDLForBad;

  mQualityHvSectorQ = Quality::Null;
  QualityCheckerHV qualityCheckerHvSectorQ;
  qualityCheckerHvSectorQ.resetHV();
  qualityCheckerHvSectorQ.mMaxBadHVForMedium = mMaxBadHVForMedium;
  qualityCheckerHvSectorQ.mMaxBadHVForBad = mMaxBadHVForBad;

  mQualityBigMap = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    if (matchHistName(mo->getName(), m_hHmpPadOccPrf_HistName)) {
      TProfile* h = getHisto<TProfile>(mo);
      if (h) {
        auto q = check_hHmpPadOccPrf(h);
        qualityCheckerOccupancy.addCheckResultDDL(q);
      }
    }

    if (matchHistName(mo->getName(), m_hBusyTime_HistName)) {
      TProfile* h = getHisto<TProfile>(mo);
      if (h && h->GetEntries() > 0) {
        auto q = check_hBusyTime(h);
        qualityCheckerBusyTime.addCheckResultDDL(q);
      }
    }

    if (matchHistName(mo->getName(), m_hEventSize_HistName)) {
      TProfile* h = getHisto<TProfile>(mo);
      if (h) {
        auto q = check_hEventSize(h);
        qualityCheckerEventSize.addCheckResultDDL(q);
      }
    }

    if (matchHistName(mo->getName(), m_hHmpHvSectorQ_HistName)) {
      TH2F* h = getHisto<TH2F>(mo);
      if (h && h->GetEntries() > 0) {
        auto q = check_hHmpHvSectorQ(h);
        qualityCheckerHvSectorQ.addCheckResultHV(q);
        qualityHvSectorQ = q;
        // Un-enable sectors we know are not working
        qualityHvSectorQ[2] = Quality::Null;
        qualityHvSectorQ[3] = Quality::Null;
        qualityHvSectorQ[7] = Quality::Null;
        qualityHvSectorQ[16] = Quality::Null;
        qualityHvSectorQ[24] = Quality::Null;
        qualityHvSectorQ[31] = Quality::Null;
        qualityHvSectorQ[34] = Quality::Null;
      }
    }
  }

  // compute the aggregated quality
  qualityCheckerOccupancy.mQualityDDL[11] = Quality::Null;
  mQualityOccupancy = qualityCheckerOccupancy.getQualityDDL();
  qualityCheckerBusyTime.mQualityDDL[11] = Quality::Null;
  mQualityBusyTime = qualityCheckerBusyTime.getQualityDDL();
  qualityCheckerEventSize.mQualityDDL[11] = Quality::Null;
  mQualityEventSize = qualityCheckerEventSize.getQualityDDL();
  qualityCheckerHvSectorQ.mQualityHV[2] = Quality::Null;
  qualityCheckerHvSectorQ.mQualityHV[3] = Quality::Null;
  qualityCheckerHvSectorQ.mQualityHV[7] = Quality::Null;
  qualityCheckerHvSectorQ.mQualityHV[16] = Quality::Null;
  qualityCheckerHvSectorQ.mQualityHV[24] = Quality::Null;
  qualityCheckerHvSectorQ.mQualityHV[31] = Quality::Null;
  qualityCheckerHvSectorQ.mQualityHV[34] = Quality::Null;
  mQualityHvSectorQ = qualityCheckerHvSectorQ.getQualityHV();
  mQualityBigMap = mQualityHvSectorQ; // <-- reasonable assumption

  // Final quality result
  Quality result = Quality::Null;
  if (mQualityOccupancy == Quality::Bad || mQualityBusyTime == Quality::Bad || mQualityEventSize == Quality::Bad || mQualityHvSectorQ == Quality::Bad) {
    result = Quality::Bad;
  } else if (mQualityOccupancy == Quality::Medium || mQualityBusyTime == Quality::Medium || mQualityEventSize == Quality::Medium || mQualityHvSectorQ == Quality::Medium) {
    result = Quality::Medium;
  } else if (mQualityOccupancy == Quality::Good || mQualityBusyTime == Quality::Good || mQualityEventSize == Quality::Good || mQualityHvSectorQ == Quality::Good) {
    result = Quality::Good;
  }

  // update the error messages
  mErrorMessages.clear();
  mErrorMessagesColor.clear();
  if (result == Quality::Good) {
    mErrorMessages.emplace_back("Quality: GOOD");
    mErrorMessagesColor.emplace_back(kGreen + 2);
  } else if (result == Quality::Medium) {
    mErrorMessages.emplace_back("Quality: MEDIUM");
    mErrorMessagesColor.emplace_back(kOrange);
    mErrorMessages.emplace_back("Add Logbook entry");
    mErrorMessagesColor.emplace_back(kOrange);
  } else if (result == Quality::Bad) {
    mErrorMessages.emplace_back("Quality: BAD");
    mErrorMessagesColor.emplace_back(kRed);
    mErrorMessages.emplace_back("Contact HMPID on-call");
    mErrorMessagesColor.emplace_back(kRed);
  } else if (result == Quality::Null) {
    mErrorMessages.emplace_back("Quality: NULL\n");
    mErrorMessagesColor.emplace_back(kBlack);
  }

  mErrorMessages.emplace_back("========================================\n");
  mErrorMessagesColor.emplace_back(kBlack);

  if (mQualityOccupancy == Quality::Null) {
    mErrorMessages.emplace_back("Occuapncy: Empty DDLs, plot not filled");
    mErrorMessagesColor.emplace_back(kBlack);
  } else if (mQualityOccupancy == Quality::Bad) {
    mErrorMessages.emplace_back("Occupancy: Too large/small for many DDLs");
    mErrorMessagesColor.emplace_back(kRed);
  } else if (mQualityOccupancy == Quality::Medium) {
    mErrorMessages.emplace_back("Occupancy: Too large/small for some DDLs");
    mErrorMessagesColor.emplace_back(kOrange);
  } else if (mQualityOccupancy == Quality::Good) {
    mErrorMessages.emplace_back("Occupancy: Good");
    mErrorMessagesColor.emplace_back(kGreen + 2);
  }

  if (mQualityEventSize == Quality::Null) {
    mErrorMessages.emplace_back("Event size: Empty DDLs, plot not filled");
    mErrorMessagesColor.emplace_back(kBlack);
  } else if (mQualityEventSize == Quality::Bad) {
    mErrorMessages.emplace_back("Event size: Too large/small for many DDLs");
    mErrorMessagesColor.emplace_back(kRed);
  } else if (mQualityEventSize == Quality::Medium) {
    mErrorMessages.emplace_back("Event size: Too large/small for some DDLs");
    mErrorMessagesColor.emplace_back(kOrange);
  } else if (mQualityEventSize == Quality::Good) {
    mErrorMessages.emplace_back("Event size: Good");
    mErrorMessagesColor.emplace_back(kGreen + 2);
  }

  if (mQualityBusyTime == Quality::Null) {
    mErrorMessages.emplace_back("Busy time: Empty DDLs, plot not filled");
    mErrorMessagesColor.emplace_back(kBlack);
  } else if (mQualityBusyTime == Quality::Bad) {
    mErrorMessages.emplace_back("Busy time: Too large/small for many DDLs");
    mErrorMessagesColor.emplace_back(kRed);
  } else if (mQualityBusyTime == Quality::Medium) {
    mErrorMessages.emplace_back("Busy time: Too large/small for some DDLs");
    mErrorMessagesColor.emplace_back(kOrange);
  } else if (mQualityBusyTime == Quality::Good) {
    mErrorMessages.emplace_back("Busy time: Good");
    mErrorMessagesColor.emplace_back(kGreen + 2);
  }

  if (mQualityHvSectorQ == Quality::Null) {
    mErrorMessages.emplace_back("HV sectors: Waiting for more entries");
    mErrorMessagesColor.emplace_back(kBlack);
  } else if (mQualityHvSectorQ == Quality::Bad) {
    mErrorMessages.emplace_back("HV and Map: Too few many HV sectors");
    mErrorMessagesColor.emplace_back(kRed);
  } else if (mQualityHvSectorQ == Quality::Medium) {
    mErrorMessages.emplace_back("HV and Map: Too few some HV sectors");
    mErrorMessagesColor.emplace_back(kOrange);
  } else if (mQualityHvSectorQ == Quality::Good) {
    mErrorMessages.emplace_back("HV and Map: Good");
    mErrorMessagesColor.emplace_back(kGreen + 2);
  }

  return result;
}

void HmpidRawChecks::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  if (mo->getName().find("CheckerMessages") != std::string::npos) {
    auto* canvas = dynamic_cast<TCanvas*>(mo->getObject());
    if (!canvas) {
      return;
    }
    canvas->cd();

    TPaveText* msg = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    msg->SetTextSize(0.8);
    msg->SetTextFont(22);
    for (std::size_t i = 0; i < mErrorMessages.size(); ++i) {
      msg->AddText(mErrorMessages[i].c_str());
      ((TText*)msg->GetListOfLines()->Last())->SetTextColor(mErrorMessagesColor[i]);
    }

    msg->SetBorderSize(0);
    msg->SetFillColor(kWhite);
    msg->Draw();
  }

  // Occupancy
  if (mo->getName().find("hHmpPadOccPrf") != std::string::npos) {
    TProfile* h = getHisto<TProfile>(mo);
    if (!h) {
      return;
    }
    float scaleMin{ 0 };
    float scaleMax{ 0 };
    scaleMin = 0; // mMinOccupancy
    scaleMax = mMaxOccupancy * 2.0;
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);

    // draw horizontal limits
    TGraph* line_max = new TGraph(2);
    line_max->SetPoint(0, h->GetXaxis()->GetXmin(), mMaxOccupancy);
    line_max->SetPoint(1, h->GetXaxis()->GetXmax(), mMaxOccupancy);
    line_max->SetLineColor(kBlue);
    line_max->SetLineStyle(kDashed);
    line_max->SetDrawOption("L");
    h->GetListOfFunctions()->Add(line_max);
    TGraph* line_min = new TGraph(2);
    line_min->SetPoint(0, h->GetXaxis()->GetXmin(), mMinOccupancy);
    line_min->SetPoint(1, h->GetXaxis()->GetXmax(), mMinOccupancy);
    line_min->SetLineColor(kBlue);
    line_min->SetLineStyle(kDashed);
    line_min->SetDrawOption("L");
    h->GetListOfFunctions()->Add(line_min);

    if (mQualityOccupancy == Quality::Good) { // checkResult
      h->SetFillColor(kGreen);

      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Good", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kGreen);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);

    } else if (mQualityOccupancy == Quality::Bad) { // checkResult
      h->SetFillColor(kRed);

      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Bad", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kRed);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);

    } else if (mQualityOccupancy == Quality::Medium) { // checkResult
      h->SetFillColor(kOrange);

      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Medium", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kOrange);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    }
    h->SetLineColor(kBlack);
  }

  // Busy time
  if (mo->getName().find("hBusyTime") != std::string::npos) {
    TProfile* h = getHisto<TProfile>(mo);
    if (!h) {
      return;
    }
    float scaleMin{ 0 };
    float scaleMax{ 0 };
    scaleMin = 0; // mMinBusyTime
    scaleMax = mMaxBusyTime * 2.0;
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);

    // draw horizontal limits
    TGraph* line_max = new TGraph(2);
    line_max->SetPoint(0, h->GetXaxis()->GetXmin(), mMaxBusyTime);
    line_max->SetPoint(1, h->GetXaxis()->GetXmax(), mMaxBusyTime);
    line_max->SetLineColor(kBlue);
    line_max->SetLineStyle(kDashed);
    line_max->SetDrawOption("L");
    h->GetListOfFunctions()->Add(line_max);
    TGraph* line_min = new TGraph(2);
    line_min->SetPoint(0, h->GetXaxis()->GetXmin(), mMinBusyTime);
    line_min->SetPoint(1, h->GetXaxis()->GetXmax(), mMinBusyTime);
    line_min->SetLineColor(kBlue);
    line_min->SetLineStyle(kDashed);
    line_min->SetDrawOption("L");
    h->GetListOfFunctions()->Add(line_min);

    if (mQualityBusyTime == Quality::Good) { // checkResult
      h->SetFillColor(kGreen);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Good", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kGreen);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityBusyTime == Quality::Bad) { // checkResult
      h->SetFillColor(kRed);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Bad", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kRed);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityBusyTime == Quality::Medium) { // checkResult
      h->SetFillColor(kOrange);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Medium", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kOrange);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    }
    h->SetLineColor(kBlack);
  }

  // Event size
  if (mo->getName().find("hEventSize") != std::string::npos) {
    TProfile* h = getHisto<TProfile>(mo);
    if (!h) {
      return;
    }
    float scaleMin{ 0 };
    float scaleMax{ 0 };
    scaleMin = 0; // mMinEventSize
    scaleMax = mMaxEventSize * 2.0;
    h->SetMinimum(scaleMin);
    h->SetMaximum(scaleMax);

    // draw horizontal limits
    TGraph* line_max = new TGraph(2);
    line_max->SetPoint(0, h->GetXaxis()->GetXmin(), mMaxEventSize);
    line_max->SetPoint(1, h->GetXaxis()->GetXmax(), mMaxEventSize);
    line_max->SetLineColor(kBlue);
    line_max->SetLineStyle(kDashed);
    line_max->SetDrawOption("L");
    h->GetListOfFunctions()->Add(line_max);
    TGraph* line_min = new TGraph(2);
    line_min->SetPoint(0, h->GetXaxis()->GetXmin(), mMinEventSize);
    line_min->SetPoint(1, h->GetXaxis()->GetXmax(), mMinEventSize);
    line_min->SetLineColor(kBlue);
    line_min->SetLineStyle(kDashed);
    line_min->SetDrawOption("L");
    h->GetListOfFunctions()->Add(line_min);

    if (mQualityEventSize == Quality::Good) { // checkResult
      h->SetFillColor(kGreen);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Good", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kGreen);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityEventSize == Quality::Bad) { // checkResult
      h->SetFillColor(kRed);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Bad", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kRed);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityEventSize == Quality::Medium) { // checkResult
      h->SetFillColor(kOrange);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Medium", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kOrange);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    }
    h->SetLineColor(kBlack);
  }

  if (mo->getName().find("hHmpHvSectorQ") != std::string::npos) {
    TH2F* h = getHisto<TH2F>(mo);
    if (!h) {
      return;
    }
    if (mQualityHvSectorQ == Quality::Good) { // checkResult
      h->SetFillColor(kGreen);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Good", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kGreen);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityHvSectorQ == Quality::Bad) { // checkResult
      h->SetFillColor(kRed);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Bad", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kRed);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityHvSectorQ == Quality::Medium) { // checkResult
      h->SetFillColor(kOrange);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Medium", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kOrange);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    }
  }

  if (mo->getName().find("hHmpBigMap_profile") != std::string::npos) {
    TProfile2D* h = getHisto<TProfile2D>(mo);
    if (!h) {
      return;
    }
    if (mQualityBigMap == Quality::Good) { // checkResult
      h->SetFillColor(kGreen);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Good", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kGreen);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityBigMap == Quality::Bad) { // checkResult
      h->SetFillColor(kRed);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Bad", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kRed);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    } else if (mQualityBigMap == Quality::Medium) { // checkResult
      h->SetFillColor(kOrange);
      TLegend* pave = new TLegend(0.65, 0.75, 0.85, 0.85);
      pave->AddEntry((TObject*)0, "Medium", "");
      pave->SetLineWidth(3);
      pave->SetTextAlign(22);
      pave->SetFillColor(kOrange);
      pave->SetBorderSize(1);
      pave->SetLineColor(kBlack);
      pave->SetTextSize(0.07);
      pave->SetTextFont(22);
      pave->SetDrawOption("same");
      h->GetListOfFunctions()->Add(pave);
    }
  }

  // Check HV
  if (mo->getName().find("hCheckHV") != std::string::npos) {
    TH2F* h = getHisto<TH2F>(mo);
    if (!h) {
      return;
    }
    TString report_warning = "Few entries:";
    TString report_good = "Good";
    TString report_null = "Null";
    int counter_bad = 0;
    int counter_all = 0;
    for (int i_module = 0; i_module < 42; i_module++) {
      if (qualityHvSectorQ[i_module] == Quality::Good) {
        h->SetBinContent(i_module + 1, h->GetYaxis()->FindBin("Good"), 1);
        h->SetBinContent(i_module + 1, h->GetYaxis()->FindBin("Bad"), 0);
        h->SetBinContent(i_module + 1, h->GetYaxis()->FindBin("Null"), -0.001);
        counter_all++;
      } else if (qualityHvSectorQ[i_module] == Quality::Bad) {
        h->SetBinContent(i_module + 1, h->GetYaxis()->FindBin("Good"), 0);
        h->SetBinContent(i_module + 1, h->GetYaxis()->FindBin("Bad"), 2);
        h->SetBinContent(i_module + 1, h->GetYaxis()->FindBin("Null"), -0.001);
        counter_all++;
        counter_bad++;
        report_warning += Form(" %d", i_module);
      }
    }
    TLegend* pave = new TLegend(0.13, 0.75, 0.87, 0.85);
    if (counter_bad == 0 && counter_all > 0) {
      pave->SetFillColor(kGreen);
      pave->AddEntry((TObject*)0, report_good, "");
    } else if (counter_bad > 0 && counter_all > 0) {
      pave->SetFillColor(kOrange);
      pave->AddEntry((TObject*)0, report_warning, "");
    } else {
      pave->SetFillColor(kWhite);
      pave->AddEntry((TObject*)0, report_null, "");
    }
    pave->SetLineWidth(3);
    pave->SetTextAlign(22);
    // pave->SetFillColor(kRed);
    pave->SetBorderSize(1);
    pave->SetLineColor(kBlack);
    pave->SetTextSize(0.07);
    pave->SetTextFont(22);
    pave->SetDrawOption("same");
    h->GetListOfFunctions()->Add(pave);
  }
}

} // namespace o2::quality_control_modules::hmpid
