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

#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCH/PhysicsCheck.h"
#include "MCH/GlobalHistogram.h"

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

PhysicsCheck::PhysicsCheck()
{
  mPrintLevel = 0;
  mMinOccupancy = 0.001;
  mMaxOccupancy = 1.00;

  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();
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
}

bool PhysicsCheck::checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel)
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

Quality PhysicsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName().find("Occupancy_Elec") != std::string::npos) {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (!h)
        return result;

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbinsy = h->GetYaxis()->GetNbins();
        int nbad = 0;
        int npads = 0;
        for (int i = 1; i <= nbinsx; i++) {
          int index = i - 1;
          int ds_addr = (index % 40);
          int link_id = (index / 40) % 12;
          int fee_id = index / (12 * 40);

          for (int j = 1; j <= nbinsy; j++) {
            int chan_addr = j - 1;

            if (!checkPadMapping(fee_id, link_id, ds_addr, chan_addr)) {
              continue;
            }
            npads += 1;

            Float_t occupancy = h->GetBinContent(i, j);
            if (occupancy >= mMinOccupancy && occupancy <= mMaxOccupancy) {
              continue;
            }

            nbad += 1;

            if (mPrintLevel >= 1) {
              std::cout << "Channel with unusual occupancy read from OccupancyElec histogrm: fee_id = " << fee_id << ", link_id = " << link_id << ", ds_addr = " << ds_addr << " , chan_addr = " << chan_addr << " with an occupancy of " << occupancy << std::endl;
            }
          }
        }
        std::cout << fmt::format("Npads {}  Nbad {}   Frac {}", npads, nbad, nbad / npads) << std::endl;
        if (nbad < 0.1 * npads)
          result = Quality::Good;
        else
          result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string PhysicsCheck::getAcceptedType() { return "TH1"; }

void PhysicsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // std::cout<<"===================================="<<std::endl;
  // std::cout<<"PhysicsCheck::beautify() called"<<std::endl;
  // std::cout<<"===================================="<<std::endl;
  if (mo->getName().find("Occupancy_Elec") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    h->SetMinimum(0);
    h->SetMaximum(10);
    TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

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

  if (mo->getName().find("MeanOccupancy") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    // disable ticks on vertical axis
    h->GetYaxis()->SetTickLength(0);
    h->SetMaximum(h->GetMaximum() * 1.2);
    // draw chamber delimiters
    for (int demin = 200; demin <= 1000; demin += 100) {
      float xpos = static_cast<float>(getDEindex(demin)) - 0.5;
      std::cout << "DEmin " << demin << "  ID " << getDEindex(demin) << std::endl;
      TLine* delimiter = new TLine(xpos, 0, xpos, 1.1 * h->GetMaximum());
      delimiter->SetLineColor(kBlack);
      delimiter->SetLineStyle(kDashed);
      h->GetListOfFunctions()->Add(delimiter);

      if (demin < 600) {
        float x1 = static_cast<float>(getDEindex(demin - 100));
        float x2 = static_cast<float>(getDEindex(demin));
        float x0 = (x1 + x2) / 2;
        TText* msg = new TText(x0, 0.88 * h->GetMaximum(), TString::Format("CH%d", (demin - 1) / 100));
        msg->SetTextAngle(90);
        h->GetListOfFunctions()->Add(msg);
      } else {
        float x1 = static_cast<float>(getDEindex(demin - 100));
        float x2 = static_cast<float>(getDEindex(demin));
        float x0 = (x1 + x2) / 2;
        TText* msg = new TText(x0, 0.95 * h->GetMaximum(), TString::Format("CH%d", (demin - 1) / 100));
        msg->SetTextAlign(22);
        h->GetListOfFunctions()->Add(msg);
      }
    }

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
}

} // namespace o2::quality_control_modules::muonchambers
