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
#include "EMCAL/NumPhysTriggCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TCanvas.h>
#include <TGraph.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TList.h>
#include <TLine.h>
#include <TRobustEstimator.h>
#include <ROOT/TSeq.hxx>
#include <iostream>
#include <vector>

using namespace std;

namespace o2::quality_control_modules::emcal
{

void NumPhysTriggCheck::configure()
{
  // configure threshold-based checkers for bad quality
  auto fracToMaxGood = mCustomParameters.find("FracToMaxGood");
  if (fracToMaxGood != mCustomParameters.end()) {
    try {
      mFracToMaxGood = std::stof(fracToMaxGood->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << fmt::format("Value {} not a float", fracToMaxGood->second.data()) << ENDM;
    }
  }
}

Quality NumPhysTriggCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Good;

  if (mo->getName().find("NPhysTriggersTFSlice") != std::string::npos) {
    auto Can = dynamic_cast<TCanvas*>(mo->getObject());
    if (Can == nullptr) {
      return Quality::Null;
    }
    TGraph* gr = nullptr;
    TList* primitives = Can->GetListOfPrimitives();
    for (TObject* obj : *primitives) {
      if (obj->InheritsFrom("TGraph")) {
        gr = (TGraph*)obj;
        break;
      }
    }
    if (gr == nullptr) {
      return Quality::Null;
    }
    if (gr->GetN() == 0) {
      return Quality::Bad;
    }
    double maxVal = 0.;
    for (int i = 0; i < gr->GetN(); ++i) {
      if (gr->GetPointY(i) > maxVal) {
        maxVal = gr->GetPointY(i);
      }
    }
    double currentVal = gr->GetPointY(gr->GetN() - 1);
    if (currentVal < mFracToMaxGood * maxVal) {
      result = Quality::Bad;
    }
  }

  return result;
}

std::string NumPhysTriggCheck::getAcceptedType() { return "TCanvas"; }

void NumPhysTriggCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("NPhysTriggersTFSlice") != std::string::npos) {
    auto Can = dynamic_cast<TCanvas*>(mo->getObject());
    if (Can == nullptr) {
      return;
    }

    Can->cd();
    TPaveText* msg = new TPaveText(0.17, 0.2, 0.5, 0.3, "NDC");
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      //
      msg->Clear();
      msg->AddText("Data quality: GOOD");
      msg->SetFillColor(kGreen);
      msg->Draw("same");
      Can->Update();
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad << ENDM";
      msg->Clear();
      msg->AddText("Data quality: BAD");
      msg->SetFillColor(kRed);
      msg->Draw("same");
      Can->Update();
    }
  }
}
} // namespace o2::quality_control_modules::emcal
