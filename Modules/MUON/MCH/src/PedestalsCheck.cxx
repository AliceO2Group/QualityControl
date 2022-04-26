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
/// \file   PedestalsCheck.cxx
/// \author Andrea Ferrero
///

#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCH/PedestalsCheck.h"

#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>
#include <TList.h>
#include <TMath.h>
#include <TPaveText.h>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

PedestalsCheck::PedestalsCheck()
  : mMinPedestal(50.f), mMaxPedestal(100.f), mMinGoodFraction(0.9), mMinGoodFractionPerDE(0.5), mPedestalsPlotScaleMin(40), mPedestalsPlotScaleMax(250), mNoisePlotScaleMin(0), mNoisePlotScaleMax(1.5)
{
  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();
}

PedestalsCheck::~PedestalsCheck() {}

void PedestalsCheck::configure()
{
  if (auto param = mCustomParameters.find("MinPedestal"); param != mCustomParameters.end()) {
    mMinPedestal = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxPedestal"); param != mCustomParameters.end()) {
    mMaxPedestal = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinGoodFractionPerDE"); param != mCustomParameters.end()) {
    mMinGoodFractionPerDE = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MinGoodFraction"); param != mCustomParameters.end()) {
    mMinGoodFraction = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("PedestalsPlotScaleMin"); param != mCustomParameters.end()) {
    mPedestalsPlotScaleMin = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("PedestalsPlotScaleMax"); param != mCustomParameters.end()) {
    mPedestalsPlotScaleMax = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("NoisePlotScaleMin"); param != mCustomParameters.end()) {
    mNoisePlotScaleMin = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("NoisePlotScaleMax"); param != mCustomParameters.end()) {
    mNoisePlotScaleMax = std::stof(param->second);
  }
}

bool PedestalsCheck::checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel, int& deId)
{
  uint16_t solarId = -1;
  int dsIddet = -1;
  int padId = -1;

  o2::mch::raw::FeeLinkId feeLinkId{ feeId, linkId };

  if (auto opt = mFeeLink2SolarMapper(feeLinkId); opt.has_value()) {
    solarId = opt.value();
  }
  if (solarId < 0 || solarId > 1023) {
    return false;
  }

  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(eLinkId / 5), static_cast<uint8_t>(eLinkId % 5) };

  if (auto opt = mElec2DetMapper(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    dsIddet = dsDetId.dsId();
    deId = dsDetId.deId();
  }

  if (deId < 0 || dsIddet < 0) {
    return false;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  padId = segment.findPadByFEE(dsIddet, int(channel));

  if (padId < 0) {
    return false;
  }
  return true;
}

Quality PedestalsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  static constexpr int sDeMax = 1100;
  double nPadsPerDE[sDeMax + 1] = { 0 };
  double nGoodPerDE[sDeMax + 1] = { 0 };

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName().find("Pedestals_Elec") != std::string::npos) {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (!h)
        return result;

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
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
            int deId = -1;

            if (!checkPadMapping(fee_id, link_id, ds_addr, chan_addr, deId)) {
              continue;
            }
            if (deId < 0 || deId > sDeMax) {
              continue;
            }
            npads += 1;
            nPadsPerDE[deId] += 1;
            Float_t ped = h->GetBinContent(i, j);
            if (ped >= mMinPedestal && ped <= mMaxPedestal) {
              ngood += 1;
              nGoodPerDE[deId] += 1;
            }
          }
        }

        result = Quality::Good;

        if (ngood < mMinGoodFraction * npads) {
          result = Quality::Bad;
        }
        for (int deId = 0; deId <= sDeMax; deId++) {
          if (nPadsPerDE[deId] == 0) {
            continue;
          }
          if (nGoodPerDE[deId] < mMinGoodFractionPerDE * nPadsPerDE[deId]) {
            result = Quality::Bad;
            break;
          }
        }
      }
    }
  }
  return result;
}

std::string PedestalsCheck::getAcceptedType() { return "TH1"; }

static void updateTitle(TH1* hist, std::string suffix)
{
  if (!hist) return;
  TString title = hist->GetTitle();
  title.Append(" ");
  title.Append(suffix.c_str());
  hist->SetTitle(title);
}

static std::string getCurrentTime()
{
  time_t t ;
  time( &t );

  struct tm *tmp ;
  tmp = localtime( &t );

  char timestr[500];
  strftime(timestr, sizeof(timestr), "(%x - %X)", tmp);

  std::string result = timestr;
  return result;
}

void PedestalsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if (mo->getName().find("Pedestals_Elec") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetOption("colz");
    h->SetMinimum(mPedestalsPlotScaleMin);
    h->SetMaximum(mPedestalsPlotScaleMax);
    TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->SetBorderSize(0);

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("All pedestals within limits: OK!!!");
      msg->SetFillColor(kGreen);
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
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

  if (mo->getName().find("Noise_Elec") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    if (!h)
      return;
    h->SetOption("colz");
    h->SetMinimum(mNoisePlotScaleMin);
    h->SetMaximum(mNoisePlotScaleMax);

    TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->SetBorderSize(0);

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("All pedestals within limits: OK!!!");
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

  if ((mo->getName().find("Pedestals_ST12") != std::string::npos) ||
      (mo->getName().find("Pedestals_ST345") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    h->SetMinimum(mPedestalsPlotScaleMin);
    h->SetMaximum(mPedestalsPlotScaleMax);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if ((mo->getName().find("Noise_ST12") != std::string::npos) ||
      (mo->getName().find("Noise_ST345") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    h->SetMinimum(mNoisePlotScaleMin);
    h->SetMaximum(mNoisePlotScaleMax);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }

  if (mo->getName().find("Pedestals_DE") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    h->SetMinimum(mPedestalsPlotScaleMin);
    h->SetMaximum(mPedestalsPlotScaleMax);
  }

  if (mo->getName().find("Noise_DE") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    h->SetMinimum(mNoisePlotScaleMin);
    h->SetMaximum(mNoisePlotScaleMax);
  }
}

} // namespace o2::quality_control_modules::muonchambers
