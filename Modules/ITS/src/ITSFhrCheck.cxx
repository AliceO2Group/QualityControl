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
/// \file   ITSFhrCheck.cxx
/// \author Liang Zhang
/// \author Jian Liu
///

#include "ITS/ITSFhrCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TList.h>
#include <TPaveText.h>
#include <TH2Poly.h>

namespace o2::quality_control_modules::its
{

void ITSFhrCheck::configure(std::string) {}

Quality ITSFhrCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {
    if (iter->second->getName() == "General/ErrorPlots") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      if (h->GetMaximum() > 0) {
        result = Quality::Bad;
      } else {
        result = Quality::Good;
      }
    } else if (iter->second->getName() == "General/General_Occupancy") {
      auto* h = dynamic_cast<TH2Poly*>(iter->second->getObject());
      result = Quality::Good;
      if (h->GetMaximum() > pow(10, -6)) {
        result = Quality::Medium;
      }
      if (h->GetMaximum() > pow(10, -5)) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string ITSFhrCheck::getAcceptedType() { return "TH1"; }

void ITSFhrCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
  auto* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
  msg->SetName(Form("%s_msg", mo->GetName()));
  if (mo->getName() == "General/ErrorPlots") {
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->AddText("There is no Error found");
      msg->SetTextColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->SetTextColor(kRed);
      msg->AddText("Decoding ERROR detected");
      msg->AddText("please inform SL");
    }
  } else if (mo->getName() == "General/General_Occupancy") {
    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Quality::Good");
      msg->SetTextColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      msg->Clear();
      msg->AddText("Quality::Bad");
      msg->SetTextColor(kRed);
      msg->AddText("Max Occupancy over 10^{-5}");
      msg->AddText("Please Inform SL");
    } else if (checkResult == Quality::Medium) {
      msg->Clear();
      msg->AddText("Quality::Medium");
      msg->SetTextColor(kOrange);
      msg->AddText("Max Occupancy over 10^{-6}");
    }
  }
  h->GetListOfFunctions()->Add(msg);
}

} // namespace o2::quality_control_modules::its
