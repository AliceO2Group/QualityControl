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
/// \file   PhysicsCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCH/PhysicsCheck.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCH/GlobalHistogram.h"
#include "MUONCommon/MergeableTH2Ratio.h"

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TMath.h>
#include <TPaveText.h>
#include <TLine.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

PhysicsCheck::PhysicsCheck() : mMinOccupancy(0.001), mMaxOccupancy(1.0), mMinGoodFraction(0.9), mOccupancyPlotScaleMin(0), mOccupancyPlotScaleMax(1), mVerbose(false)
{
  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();

  mDeOccupancy.resize(getDEindexMax() + 1);
  mDeOccupancyOnCycle.resize(getDEindexMax() + 1);
}

PhysicsCheck::~PhysicsCheck() {}

void PhysicsCheck::configure()
{
  if (auto param = mCustomParameters.find("MinOccupancy"); param != mCustomParameters.end()) {
    mMinOccupancy = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxOccupancy"); param != mCustomParameters.end()) {
    mMaxOccupancy = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinGoodFraction"); param != mCustomParameters.end()) {
    mMinGoodFraction = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("OccupancyPlotScaleMin"); param != mCustomParameters.end()) {
    mOccupancyPlotScaleMin = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("OccupancyPlotScaleMax"); param != mCustomParameters.end()) {
    mOccupancyPlotScaleMax = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("Verbose"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mVerbose = true;
    }
  }
}

int PhysicsCheck::checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel)
{
  uint16_t solarId = -1;
  int deId = -1;
  int dsIddet = -1;
  int padId = -1;

  o2::mch::raw::FeeLinkId feeLinkId{ feeId, linkId };

  if (auto opt = mFeeLink2SolarMapper(feeLinkId); opt.has_value()) {
    solarId = opt.value();
  }
  if (solarId < 0 || solarId > 1023) {
    return -1;
  }

  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(eLinkId / 5), static_cast<uint8_t>(eLinkId % 5) };

  if (auto opt = mElec2DetMapper(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    dsIddet = dsDetId.dsId();
    deId = dsDetId.deId();
  }

  if (deId < 0 || dsIddet < 0) {
    return -1;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  padId = segment.findPadByFEE(dsIddet, int(channel));

  if (padId < 0) {
    return -1;
  }
  return deId;
}

Quality PhysicsCheck::processFecOccupancy(MergeableTH2Ratio* hr, std::vector<double>& deOccupancy)
{
  Quality result = Quality::Null;
  // cumulative numerators and denominators for the computation of
  // the average occupancy over one detection element
  std::vector<double> deOccupancyNum(getDEindexMax() + 1);
  std::vector<double> deOccupancyDen(getDEindexMax() + 1);
  std::fill(deOccupancyNum.begin(), deOccupancyNum.end(), 0);
  std::fill(deOccupancyDen.begin(), deOccupancyDen.end(), 0);

  auto* h = dynamic_cast<TH2F*>(hr);
  if (!h) {
    return result;
  }

  if (h->GetEntries() == 0) {
    return Quality::Medium;
  }

  int nbinsx = h->GetXaxis()->GetNbins();
  int nbinsy = h->GetYaxis()->GetNbins();
  int ngood = 0;
  int npads = 0;
  for (int i = 1; i <= nbinsx; i++) {
    int index = i - 1;
    int ds_addr = (index % 40);
    int link_id = (index / 40) % 12;
    int fee_id = index / (12 * 40);

    for (int j = 1; j <= nbinsy; j++) {
      int chan_addr = j - 1;

      int de = checkPadMapping(fee_id, link_id, ds_addr, chan_addr);
      if (de < 0) {
        continue;
      }

      Float_t stat = hr->getDen()->GetBinContent(i, j);
      if (stat < 1) {
        continue;
      }

      Float_t occupancy = h->GetBinContent(i, j);
      npads += 1;
      if (occupancy >= mMinOccupancy && occupancy <= mMaxOccupancy) {
        ngood += 1;
      }

      int deIndex = getDEindex(de);
      if (deIndex >= 0) {
        deOccupancyNum[deIndex] += occupancy;
        deOccupancyDen[deIndex] += 1;
      }
    }
  }
  if (mVerbose) {
    LOGP(debug, "Npads {}  Ngood {}   Frac {}", npads, ngood, float(ngood) / float(npads));
  }

  if (ngood >= mMinGoodFraction * npads)
    result = Quality::Good;
  else
    result = Quality::Bad;

  // update the average occupancy values that will be copied
  // into the histogram bins in the beutify() method
  for (size_t de = 0; de < deOccupancyDen.size(); de++) {
    // integrated occupancies
    if (deOccupancyDen[de] > 0) {
      deOccupancy[de] = deOccupancyNum[de] / deOccupancyDen[de];
      if (de == 10) {
        std::cout << fmt::format("xxxx occupancy: {} = {} / {}", deOccupancy[de], deOccupancyNum[de], deOccupancyDen[de]) << std::endl;
      }
    } else {
      deOccupancy[de] = 0;
    }
  }

  return result;
}

Quality PhysicsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    if (mo->getName().find("Occupancy_Elec") != std::string::npos) {

      auto* hr = dynamic_cast<MergeableTH2Ratio*>(mo->getObject());
      if (!hr) {
        return result;
      }

      result = processFecOccupancy(hr, mDeOccupancy);

      if (!mHistogramOccupancyFecPrevCycle) {
        mHistogramOccupancyFecPrevCycle = std::make_shared<MergeableTH2Ratio>(*hr);
        mHistogramOccupancyFecPrevCycle->SetName("mHistogramOccupancyFecPrevCycle");
        mHistogramOccupancyFecPrevCycle->Reset();
        mHistogramOccupancyFecPrevCycle->getNum()->Reset();
        mHistogramOccupancyFecPrevCycle->getDen()->Reset();
      }

      MergeableTH2Ratio hdiff(*hr);
      hdiff.SetName("mHistogramOccupancyFecOnCycle");
      hdiff.Reset();
      hdiff.getNum()->Reset();
      hdiff.getDen()->Reset();

      hdiff.getNum()->Add(hr->getNum());
      hdiff.getNum()->Add(mHistogramOccupancyFecPrevCycle->getNum(), -1);
      hdiff.getDen()->Add(hr->getDen());
      hdiff.getDen()->Add(mHistogramOccupancyFecPrevCycle->getDen(), -1);
      hdiff.update();

      processFecOccupancy(&hdiff, mDeOccupancyOnCycle);

      // update previous cycle plot
      mHistogramOccupancyFecPrevCycle->Reset();
      mHistogramOccupancyFecPrevCycle->getNum()->Reset();
      mHistogramOccupancyFecPrevCycle->getDen()->Reset();
      mHistogramOccupancyFecPrevCycle->getNum()->Add(hr->getNum());
      mHistogramOccupancyFecPrevCycle->getDen()->Add(hr->getDen());
    }
  }
  return result;
}

std::string PhysicsCheck::getAcceptedType() { return "TH1"; }

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

void PhysicsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if (mo->getName().find("Occupancy_Elec") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    h->SetMinimum(mOccupancyPlotScaleMin);
    h->SetMaximum(mOccupancyPlotScaleMax);
    TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->SetBorderSize(0);

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("All occupancies within limits: OK!!!");
      msg->SetFillColor(kGreen);

      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);

      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";

      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if ((mo->getName().find("Occupancy_ST12") != std::string::npos) ||
      (mo->getName().find("Occupancy_ST345") != std::string::npos) ||
      (mo->getName().find("OccupancyOnCycle_ST12") != std::string::npos) ||
      (mo->getName().find("OccupancyOnCycle_ST345") != std::string::npos) ||
      (mo->getName().find("Occupancy_B_XY") != std::string::npos) ||
      (mo->getName().find("Occupancy_NB_XY") != std::string::npos)) {
    std::cout << "Beautifying " << mo->getName() << std::endl;
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(mOccupancyPlotScaleMin);
    h->SetMaximum(mOccupancyPlotScaleMax);
    std::cout << fmt::format("Beautifying: GetListOfFunctions->GetEntries() {}", h->GetListOfFunctions()->GetEntries()) << std::endl;
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if (mo->getName().find("MeanOccupancy") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    // update the mean occupancy values
    if (mo->getName().find("MeanOccupancyOnCycle") != std::string::npos) {
      for (size_t deIndex = 0; deIndex < mDeOccupancy.size(); deIndex++) {
        h->SetBinContent(deIndex + 1, mDeOccupancyOnCycle[deIndex]);
        h->SetBinError(deIndex + 1, 0);
      }
    } else {
      for (size_t deIndex = 0; deIndex < mDeOccupancy.size(); deIndex++) {
        h->SetBinContent(deIndex + 1, mDeOccupancy[deIndex]);
        h->SetBinError(deIndex + 1, 0);
      }
    }

    // disable ticks on vertical axis
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0);
    h->GetYaxis()->SetTitle("occupancy (kHz)");
    h->SetMaximum(h->GetMaximum() * 1.5);

    TText* xtitle = new TText();
    xtitle->SetNDC();
    xtitle->SetText(0.87, 0.03, "chamber #");
    xtitle->SetTextSize(15);
    h->GetListOfFunctions()->Add(xtitle);

    // draw chamber delimiters
    for (int demin = 200; demin <= 1000; demin += 100) {
      float xpos = static_cast<float>(getDEindex(demin));
      TLine* delimiter = new TLine(xpos, 0, xpos, h->GetMaximum());
      delimiter->SetLineColor(kBlack);
      delimiter->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(delimiter);
    }

    // draw x-axis labels
    for (int ch = 1; ch <= 10; ch += 1) {
      float x1 = static_cast<float>(getDEindex(ch * 100));
      float x2 = (ch < 10) ? static_cast<float>(getDEindex(ch * 100 + 100)) : h->GetXaxis()->GetXmax();
      float x0 = 0.8 * (x1 + x2) / (2 * h->GetXaxis()->GetXmax()) + 0.1;
      float y0 = 0.05;
      TText* label = new TText();
      label->SetNDC();
      label->SetText(x0, y0, TString::Format("%d", ch));
      label->SetTextSize(15);
      label->SetTextAlign(22);
      h->GetListOfFunctions()->Add(label);
    }

    TPaveText* msg = new TPaveText(0.1, 0.903, 0.9, 0.945, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->SetBorderSize(0);

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("All occupancies within limits: OK!!!");
      msg->SetFillColor(kGreen);

      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);

      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";

      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }

  if (mo->getName().find("DigitOrbitInTFDE") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    // disable ticks on vertical axis
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0);
    h->GetYaxis()->SetTitle("digit orbit");

    TText* xtitle = new TText();
    xtitle->SetText(h->GetXaxis()->GetXmax() - 5, h->GetYaxis()->GetXmin() * 1.2, "chamber #");
    xtitle->SetTextSize(15);
    h->GetListOfFunctions()->Add(xtitle);

    // draw chamber delimiters
    for (int demin = 200; demin <= 1000; demin += 100) {
      float xpos = static_cast<float>(getDEindex(demin));
      TLine* delimiter = new TLine(xpos, h->GetYaxis()->GetXmin(), xpos, h->GetYaxis()->GetXmax());
      delimiter->SetLineColor(kBlack);
      delimiter->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(delimiter);
    }

    // draw x-axis labels
    for (int ch = 1; ch <= 10; ch += 1) {
      float x1 = static_cast<float>(getDEindex(ch * 100));
      float x2 = (ch < 10) ? static_cast<float>(getDEindex(ch * 100 + 100)) : h->GetXaxis()->GetXmax();
      float x0 = (x1 + x2) / 2;
      float y0 = h->GetYaxis()->GetXmin() * 1.12;
      TText* label = new TText();
      label->SetText(x0, y0, TString::Format("%d", ch));
      label->SetTextSize(15);
      label->SetTextAlign(22);
      h->GetListOfFunctions()->Add(label);
    }
  }
}

} // namespace o2::quality_control_modules::muonchambers
