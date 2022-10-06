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
/// \file   DecodingErrorsCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include "MCH/DecodingErrorsCheck.h"
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

DecodingErrorsCheck::DecodingErrorsCheck()
{
  mPrintLevel = 0;
  mMaxErrorRate = 100.00;
}

DecodingErrorsCheck::~DecodingErrorsCheck() {}

void DecodingErrorsCheck::configure()
{
  bool mDiagnostic = false;
  if (auto param = mCustomParameters.find("MaxErrorRate"); param != mCustomParameters.end()) {
    mMaxErrorRate = std::stof(param->second);
  }
}

Quality DecodingErrorsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName().find("DecodingErrorsPerFeeId") != std::string::npos) {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (!h) {
        return result;
      }

      int nbad = 0;
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
      } else {
        int nbinsx = h->GetXaxis()->GetNbins();
        int nbinsy = h->GetYaxis()->GetNbins();
        for (int i = 1; i <= nbinsx; i++) {
          for (int j = 1; j <= nbinsy; j++) {
            Float_t errorRate = h->GetBinContent(i, j);
            if (errorRate < mMaxErrorRate) {
              continue;
            }

            nbad += 1;
          }
        }
        if (nbad == 0)
          result = Quality::Good;
        else
          result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string DecodingErrorsCheck::getAcceptedType() { return "TH1"; }

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

void DecodingErrorsCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if (mo->getName().find("DecodingErrorsPerFeeId") != std::string::npos ||
      mo->getName().find("DecodingErrorsPerChamber") != std::string::npos) {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    h->SetDrawOption("colz");
    TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->SetBorderSize(0);

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("All errors within limits: OK!!!");
      msg->SetFillColor(kGreen);

      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Too many errors, call MCH on-call.");
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


  if (mo->getName().find("SynchErrorsPerDE") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    // disable ticks on vertical axis
    h->GetXaxis()->SetTickLength(0.0);
    h->GetXaxis()->SetLabelSize(0.0);
    h->GetYaxis()->SetTickLength(0);
    h->GetYaxis()->SetTitle("out-of-sync fraction");
    //h->SetMaximum(h->GetMaximum() * 1.5);

    TText* xtitle = new TText();
    xtitle->SetNDC();
    xtitle->SetText(0.87, 0.03, "chamber #");
    xtitle->SetTextSize(15);
    h->GetListOfFunctions()->Add(xtitle);

    // draw chamber delimiters
    for (int demin = 200; demin <= 1000; demin += 100) {
      float xpos = static_cast<float>(getDEindex(demin));
      TLine* delimiter = new TLine(xpos, 0, xpos, h->GetMaximum() * 1.05);
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

    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::muonchambers
