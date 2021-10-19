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
#include <TLatex.h>
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
	TLatex *text[5];
  if (mo->getName() == "General/ErrorPlots") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
	text[0] = new TLatex(0, 0, "Quality::Good");
	text[1] = new TLatex(0, -100, "There is no Error found");
	for(int i = 0; i < 2; ++i) {
		text[i]->SetTextAlign(23);
		text[i]->SetTextSize(0.08);
		text[i]->SetTextColor(kGreen);
		h->GetListOfFunctions()->Add(text[i]);
	}
    } else if (checkResult == Quality::Bad) {
	text[0] = new TLatex(0, 100, "Quality::Bad");
	text[1] = new TLatex(0, 0, "Decoding ERROR detected");
	text[2] = new TLatex(0, -100, "please inform SL");
        for(int i = 0; i < 3; ++i) {
                text[i]->SetTextAlign(23);
                text[i]->SetTextSize(0.08);
                text[i]->SetTextColor(kRed);
                h->GetListOfFunctions()->Add(text[i]);
        }
    }
  } else if (mo->getName() == "General/General_Occupancy") {
    auto* h = dynamic_cast<TH2Poly*>(mo->getObject());
    auto* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    msg->SetName(Form("%s_msg", mo->GetName()));
    if (checkResult == Quality::Good) {
	text[0] = new TLatex(0, 0, "Quality::Good");
	text[0]->SetTextAlign(23);
	text[0]->SetTextSize(0.08);
	text[0]->SetTextColor(kGreen);
	h->GetListOfFunctions()->Add(text[0]);
    } else if (checkResult == Quality::Bad) {
	text[0] = new TLatex(0, 100, "Quality::Bad");
	text[1] = new TLatex(0, 0, "Max Occupancy over 10^{-5}");
	text[2] = new TLatex(0, -100, "or ERROR detected, Please Inform SL");
        for(int i = 0; i < 3; ++i) {
                text[i]->SetTextAlign(23);
                text[i]->SetTextSize(0.08);
                text[i]->SetTextColor(kRed);
                h->GetListOfFunctions()->Add(text[i]);
        }
    } else if (checkResult == Quality::Medium) {
	text[0] = new TLatex(0, 0, "Quality::Medium");
	text[1] = new TLatex(0, -100, "Max Occupancy over 10^{-6}");
        for(int i = 0; i < 2; ++i) {
                text[i]->SetTextAlign(23);
                text[i]->SetTextSize(0.08);
                text[i]->SetTextColor(kOrange);
                h->GetListOfFunctions()->Add(text[i]);
        }
    }
  }
}

} // namespace o2::quality_control_modules::its
