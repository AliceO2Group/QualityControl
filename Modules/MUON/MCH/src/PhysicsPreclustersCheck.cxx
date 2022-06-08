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
/// \file   PhysicsPreclustersCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCH/PhysicsPreclustersCheck.h"
#include "MCH/GlobalHistogram.h"

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TLine.h>
#include <TMath.h>
#include <TPaveText.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

PhysicsPreclustersCheck::PhysicsPreclustersCheck() : mMinPseudoeff(0.5), mMaxPseudoeff(1.0)
{
}

PhysicsPreclustersCheck::~PhysicsPreclustersCheck() {}

void PhysicsPreclustersCheck::configure()
{
  if (auto param = mCustomParameters.find("MinPseudoefficiency"); param != mCustomParameters.end()) {
    mMinPseudoeff = std::stof(param->second);
  }
  if (auto param = mCustomParameters.find("MaxPseudoefficiency"); param != mCustomParameters.end()) {
    mMaxPseudoeff = std::stof(param->second);
  }
}

Quality PhysicsPreclustersCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if ((mo->getName().find("MeanPseudoeffPerDE_B") != std::string::npos) ||
        (mo->getName().find("MeanPseudoeffPerDE_NB") != std::string::npos)) {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        return result;
      }

      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbad = 0;
        for (int i = 1; i <= nbinsx; i++) {
          Float_t eff = h->GetBinContent(i);
          if (eff < mMinPseudoeff || eff > mMaxPseudoeff) {
            nbad += 1;
          }
        }
        if (nbad < 1) {
          result = Quality::Good;
          std::cout << "GOOD" << endl;
        } else {
          result = Quality::Bad;
          std::cout << "BAD" << endl;
        }
      }
    }
  }

  return result;
}

std::string PhysicsPreclustersCheck::getAcceptedType() { return "TH1"; }

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
  strftime(timestr, sizeof(timestr), "(%x - %X)", tmp);

  std::string result = timestr;
  return result;
}

void PhysicsPreclustersCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if ((mo->getName().find("MeanPseudoeffPerDE_B") != std::string::npos) ||
      (mo->getName().find("MeanPseudoeffPerDE_NB") != std::string::npos) ||
      (mo->getName().find("PreclustersPerDE") != std::string::npos) ||
      (mo->getName().find("PreclustersSignalPerDE") != std::string::npos)) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    // disable ticks on axis
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0);
    h->GetYaxis()->SetTitle("efficiency");

    h->SetMinimum(0);
    if ((mo->getName().find("MeanPseudoeffPerDE_B") != std::string::npos) ||
        (mo->getName().find("MeanPseudoeffPerDE_NB") != std::string::npos)) {
      h->SetMaximum(2);
    }

    TText* xtitle = new TText();
    xtitle->SetNDC();
    xtitle->SetText(0.87, 0.03, "chamber #");
    xtitle->SetTextSize(15);
    h->GetListOfFunctions()->Add(xtitle);

    // draw chamber delimiters
    for (int demin = 200; demin <= 1000; demin += 100) {
      float xpos = static_cast<float>(getDEindex(demin));
      TLine* delimiter = new TLine(xpos, 0, xpos, 2);
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

    TPaveText* msg = new TPaveText(0.3, 0.9, 0.7, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("Pseudo-efficiency consistently within limits: OK!!!");
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      msg->Clear();
      msg->AddText("Call MCH on-call.");
      msg->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";
      msg->Clear();
      msg->AddText("No entries. If MCH in the run, check MCH TWiki");
      msg->SetFillColor(kYellow);
    }
    h->SetLineColor(kBlack);
  }

  if ((mo->getName().find("Pseudoeff_ST12") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_ST345") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_B_XY") != std::string::npos) ||
      (mo->getName().find("Pseudoeff_NB_XY") != std::string::npos)) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetMinimum(0);
    h->SetMaximum(1);
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0.0);
    h->GetYaxis()->SetLabelSize(0.0);
  }
}

} // namespace o2::quality_control_modules::muonchambers
