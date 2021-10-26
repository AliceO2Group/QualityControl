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

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TMath.h>
#include <TPaveText.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

PhysicsCheck::PhysicsCheck()
{
  mPrintLevel = 0;
  minOccupancy = 0.05;
  maxOccupancy = 1.00;
}

PhysicsCheck::~PhysicsCheck() {}

void PhysicsCheck::configure(std::string)
{
  //   if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
  //     minTOFrawTime = 150.; //ns
  //     maxTOFrawTime = 250.; //ns
  //   }
}

Quality PhysicsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::cout << "=================================" << std::endl;
  std::cout << "PhysicsCheck::check() called" << std::endl;
  std::cout << "=================================" << std::endl;
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
        for (int i = 1; i <= nbinsx; i++) {
          for (int j = 1; j <= nbinsy; j++) {
            Float_t occ = h->GetBinContent(i, j);
            if (occ < minOccupancy || occ >= maxOccupancy) {
              //nbad += 1;
              int ds_addr = (i % 40) - 1;
              int link_id = ((i - 1 - ds_addr) / 40) % 12;
              int fee_id = (i - 1 - ds_addr - 40 * link_id) / (12 * 40);
              int chan_addr = j - 1;

              if (mPrintLevel >= 1) {
                std::cout << "Channel with unusual occupancy read from OccupancyElec histogrm: fee_id = " << fee_id << ", link_id = " << link_id << ", ds_addr = " << ds_addr << " , chan_addr = " << chan_addr << " with an occupancy of " << occ << std::endl;
              }
            }
          }
        }
        if (nbad < 1)
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
  //std::cout<<"===================================="<<std::endl;
  //std::cout<<"PhysicsCheck::beautify() called"<<std::endl;
  //std::cout<<"===================================="<<std::endl;
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
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::muonchambers
