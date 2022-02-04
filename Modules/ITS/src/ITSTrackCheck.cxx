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
/// \file   ITSTrackCheck.cxx
/// \author Artem Isakov
/// \author Jian Liu
///

#include "ITS/ITSTrackCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include <TPaveText.h>
#include <TList.h>
#include <TH1.h>
#include <TH2.h>
#include <TText.h>
#include "TMath.h"
#include "TLatex.h"

#include <iostream>

namespace o2::quality_control_modules::its
{

void ITSTrackCheck::configure() {}

Quality ITSTrackCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = 0;
  Int_t id = 0;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {

    if (iter->second->getName() == "NClusters") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      if (h->GetMean() > 10 && h->GetMean() < 15)
        result = result.getLevel() + 1;
      else if (h->GetMean() >= 15)
        result = result.getLevel() + 2;
    }

    if (iter->second->getName() == "PhiDistribution") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      Double_t ratio = abs(h->Integral(h->FindBin(0), h->FindBin(TMath::Pi())) / h->Integral(h->FindBin(TMath::Pi()), h->FindBin(TMath::TwoPi())) - 1);
      if (ratio > 0.3)
        result = result.getLevel() + 10;
    }

    if (iter->second->getName() == "AngularDistribution") {
      auto* hAngluar = dynamic_cast<TH2D*>(iter->second->getObject());
      TH1D* projectPhi = hAngluar->ProjectionY();
      Double_t ratio = abs(projectPhi->Integral(projectPhi->FindBin(0), projectPhi->FindBin(TMath::Pi())) / projectPhi->Integral(projectPhi->FindBin(TMath::Pi()), projectPhi->FindBin(TMath::TwoPi())) - 1);
      if (ratio > 0.3)
        result = result.getLevel() + 1e2;
    }

    if (iter->second->getName() == "EtaDistribution") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      if (abs(1. - (h->Integral(1, h->FindBin(0)) / h->Integral(h->FindBin(0), h->GetNbinsX()))) > 0.3)
        result = result.getLevel() + 1e4;
    }

    if (iter->second->getName() == "VertexCoordinates") {
      TH2D* h = dynamic_cast<TH2D*>(iter->second->getObject());

      TH1D* projectY = h->ProjectionY();
      if ((projectY->Integral(1, projectY->FindBin(-0.2)) > 0) || (projectY->Integral(projectY->FindBin(0.2), projectY->GetNbinsX()) > 0))
        result = result.getLevel() + 1e5;
      TH1D* projectX = h->ProjectionX();
      if ((projectX->Integral(1, projectX->FindBin(-0.2))) > 0 || (projectX->Integral(projectX->FindBin(0.2), projectX->GetNbinsX()) > 0))
        result = result.getLevel() + 2e5;
    }

    if (iter->second->getName() == "VertexRvsZ") {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());
      TH1D* projectZ = h->ProjectionY();
      if ((projectZ->Integral(1, projectZ->FindBin(-10)) > 0) || (projectZ->Integral(projectZ->FindBin(10), projectZ->GetNbinsX()) > 0))
        result = result.getLevel() + 1e6;

      TH1D* projectR = h->ProjectionX();
      if (projectR->Integral(projectR->FindBin(0.3), projectR->GetNbinsX()) > 0)
        result = result.getLevel() + 2e6;
    }

    if (iter->second->getName() == "VertexZ") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      if ((h->FindBin(-10) != 0 && h->Integral(1, h->FindBin(-10)) > 0) || (h->FindBin(10) != (h->GetNbinsX() + 1) && h->Integral(h->FindBin(10), h->GetNbinsX()) > 0))
        result = result.getLevel() + 1e7;
    }
  }
  return result;
}

std::string ITSTrackCheck::getAcceptedType() { return "TH1D"; }

void ITSTrackCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  TString text[2];
  int textColor;

  if (mo->getName() == "NClusters") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    int histoQuality = getDigit(checkResult.getLevel(), 1);
    if (histoQuality == 0) {
      text[0] = "Quality::GOOD";
      textColor = kGreen;
    } else if (histoQuality == 1) {
      text[0] = "INFO: a track(s) has between 10 and 15 clusters";
      text[1] = "inform expert on MM";
      textColor = kYellow;
    } else {
      text[0] = "INFO: a track(s) has more than 15 clusters";
      text[1] = "call expert";
      textColor = kRed;
    }
    auto* msg = new TLatex(0.15, 0.7, histoQuality == 0 ? text[0].Data() : Form("#bf{#splitline{%s}{%s}}", text[0].Data(), text[1].Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.08);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg);
  }

  if (mo->getName() == "PhiDistribution") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    int histoQuality = getDigit(checkResult.getLevel(), 2);
    if (histoQuality == 0) {
      text[0] = "Quality::GOOD";
      textColor = kGreen;
    } else {
      text[0] = "INFO: distribution asymmetric in phi";
      text[1] = "call expert";
      textColor = kRed;
    }

    auto* msg = new TLatex(0.15, 0.25, histoQuality == 0 ? text[0].Data() : Form("#bf{#splitline{%s}{%s}}", text[0].Data(), text[1].Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.08);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg);
  }

  if (mo->getName() == "AngularDistribution") {
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    int histoQuality = getDigit(checkResult.getLevel(), 3);
    Double_t positionX, positionY;
    if (histoQuality == 0) {
      text[0] = "Quality::GOOD";
      textColor = kGreen;
      positionX = 0.02;
      positionY = 0.9;
    } else {
      text[0] = "INFO: distribution asymmetric in phi";
      text[1] = "call expert";
      textColor = kRed;
      positionX = 0.15;
      positionY = 0.7;
    }

    auto* msg = new TLatex(positionX, positionY, histoQuality == 0 ? text[0].Data() : Form("#bf{#splitline{%s}{%s}}", text[0].Data(), text[1].Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.08);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg);
  }

  if (mo->getName() == "EtaDistribution") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    int histoQuality = getDigit(checkResult.getLevel(), 5);
    if (histoQuality == 0) {
      text[0] = "Quality::GOOD";
      textColor = kGreen;
    } else {
      text[0] = "INFO: distribution asymmetric in eta";
      text[1] = "call expert";
      textColor = kRed;
    }
    auto* msg = new TLatex(0.15, 0.2, histoQuality == 0 ? text[0].Data() : Form("#bf{#splitline{%s}{%s}}", text[0].Data(), text[1].Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.08);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg);
  }

  if (mo->getName() == "VertexCoordinates") {
    Double_t positionX, positionY;
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    int histoQuality = getDigit(checkResult.getLevel(), 6);
    if (histoQuality == 0) {
      text[0] = "Quality::GOOD";
      textColor = kGreen;
      positionX = 0.02;
      positionY = 0.9;
    } else {

      if (histoQuality == 1) {
        text[0] = "INFO: vertex Y displaced > 2 mm ";
      } else if (histoQuality == 2) {
        text[0] = "INFO: vertex X displaced > 2 mm ";
      } else if (histoQuality == 3) {
        text[0] = "INFO: vertex X and Y displaced > 2 mm ";
      }

      text[1] = "Inform expert on MM";
      textColor = kRed;
      positionX = 0.15;
      positionY = 0.7;
    }
    auto* msg = new TLatex(positionX, positionY, histoQuality == 0 ? text[0].Data() : Form("#bf{#splitline{%s}{%s}}", text[0].Data(), text[1].Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.08);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg);
  }

  if (mo->getName() == "VertexRvsZ") {
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    int histoQuality = getDigit(checkResult.getLevel(), 7);

    if (histoQuality == 0) {
      text[0] = "Quality::GOOD";
      textColor = kGreen;
    } else {

      if (histoQuality == 1) {
        text[0] = "INFO: vertex distance on XY plane > 3 mm";
      } else if (histoQuality == 2) {
        text[0] = "INFO: vertex Z displaced > 10 cm";
      } else if (histoQuality == 3) {
        text[0] = "INFO: vertex Z displaced > 10 cm, XY > 3 mm";
      }
      text[1] = "Inform expert on MM";
      textColor = kRed;
    }
    auto* msg = new TLatex(0.15, 0.7, histoQuality == 0 ? text[0].Data() : Form("#bf{#splitline{%s}{%s}}", text[0].Data(), text[1].Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.08);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg);
  }

  if (mo->getName() == "VertexZ") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    int histoQuality = getDigit(checkResult.getLevel(), 8);
    if (histoQuality == 0) {
      text[0] = "Quality::GOOD";
      textColor = kGreen;
    } else {
      text[0] = "INFO: vertex z displaced > 10 cm";
      textColor = kRed;
    }

    auto* msg = new TLatex(0.15, 0.7, histoQuality == 0 ? text[0].Data() : Form("#bf{#splitline{%s}{%s}}", text[0].Data(), text[1].Data()));
    msg->SetTextColor(textColor);
    msg->SetTextSize(0.08);
    msg->SetTextFont(43);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg);
  }
}

int ITSTrackCheck::getDigit(int number, int digit)
{
  return number % (int)pow(10, digit) / (int)pow(10, digit - 1);
}

} // namespace o2::quality_control_modules::its
