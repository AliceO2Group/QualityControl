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
#include "Common/Utils.h"

namespace o2::quality_control_modules::its
{

Quality ITSTrackCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  mEtaRatio = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "EtaRatio", mEtaRatio);
  mPhiRatio = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "PhiRatio", mPhiRatio);

  Quality result = 0;
  Int_t id = 0;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {

    if (iter->second->getName() == "NClusters") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      result.set(Quality::Good);
      result.addMetadata("CheckTracks4", "good");
      result.addMetadata("CheckTracks5", "good");
      result.addMetadata("CheckTracks6", "good");
      result.addMetadata("CheckTracks7", "good");
      result.addMetadata("CheckEmpty", "good");
      result.addMetadata("CheckMean", "good");
      if (h->GetMean() < 5.2 || h->GetMean() > 6.2) {

        result.updateMetadata("CheckMean", "medium");
        result.set(Quality::Medium);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), Form("Medium: Mean (%.1f) is outside 5.2-5.9, ignore for COSMICS and TECHNICALS", h->GetMean()));
      }
      if (h->GetBinContent(h->FindBin(4)) < 1e-15) {
        result.updateMetadata("CheckTracks4", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: no tracks with 4 clusters");
      }
      if (h->GetBinContent(h->FindBin(5)) < 1e-15) {
        result.updateMetadata("CheckTracks5", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: no tracks with 5 clusters");
      }
      if (h->GetBinContent(h->FindBin(6)) < 1e-15) {
        result.updateMetadata("CheckTracks6", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: no tracks with 6 clusters");
      }
      if (h->GetBinContent(h->FindBin(7)) < 1e-15) {
        result.updateMetadata("CheckTracks7", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: no tracks with 7 clusters");
      }
      if (h->GetEntries() < 1e-15) {
        result.updateMetadata("CheckEmpty", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: no tracks!");
      }
    }

    if (iter->second->getName() == "AngularDistribution") {
      auto* hAngular = dynamic_cast<TH2D*>(iter->second->getObject());
      result.set(Quality::Good);
      result.addMetadata("CheckAngEmpty", "good");
      result.addMetadata("CheckAsymmEta", "good");
      result.addMetadata("CheckAsymmPhi", "good");
      TH1D* projectEta = hAngular->ProjectionX();
      Double_t ratioEta = abs(1. - (projectEta->Integral(1, projectEta->FindBin(0)) / projectEta->Integral(projectEta->FindBin(0), projectEta->GetNbinsX())));
      TH1D* projectPhi = hAngular->ProjectionY();
      Double_t ratioPhi = abs(projectPhi->Integral(projectPhi->FindBin(0), projectPhi->FindBin(TMath::Pi())) / projectPhi->Integral(projectPhi->FindBin(TMath::Pi()), projectPhi->FindBin(TMath::TwoPi())) - 1);
      if (hAngular->GetEntries() < 1e-15) {
        result.updateMetadata("CheckAngEmpty", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: no tracks!");
      }
      if (ratioEta > mEtaRatio) {
        result.updateMetadata("CheckAsymmEta", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: Eta asymmetry");
      }
      if (ratioPhi > mPhiRatio) {
        result.updateMetadata("CheckAsymmPhi", "bad");
        result.set(Quality::Bad);
        result.addReason(o2::quality_control::FlagReasonFactory::Unknown(), "BAD: Phi asymmetry");
      }
    }

    if (iter->second->getName() == "VertexCoordinates") {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());
      result.set(Quality::Good);
      result.addMetadata("CheckXYVertexEmpty", "good");
      result.addMetadata("CheckXVertexMean", "good");
      result.addMetadata("CheckYVertexMean", "good");
      result.addMetadata("CheckXVertexRms", "good");
      result.addMetadata("CheckYVertexRms", "good");
      if (std::abs(h->GetMean(1)) > 0.05) {
        result.updateMetadata("CheckXVertexMean", "medium");
        result.set(Quality::Medium);
      }
      if (std::abs(h->GetMean(2)) > 0.05) {
        result.updateMetadata("CheckYVertexMean", "medium");
        result.set(Quality::Medium);
      }
      if (h->GetRMS(1) > 0.1) {
        result.updateMetadata("CheckXVertexRms", "medium");
        result.set(Quality::Medium);
      }
      if (h->GetRMS(2) > 0.1) {
        result.updateMetadata("CheckYVertexRms", "medium");
        result.set(Quality::Medium);
      }
      if (h->GetEntries() < 1e-15 || (h->Integral(h->GetXaxis()->FindBin(-0.2), h->GetXaxis()->FindBin(+0.2), h->GetYaxis()->FindBin(0.05), h->GetYaxis()->FindBin(0.2)) < 1e-15)) {
        result.updateMetadata("CheckXYVertexEmpty", "bad");
        result.set(Quality::Bad);
      }
    }

    if (iter->second->getName() == "VertexRvsZ") {
      auto* h = dynamic_cast<TH2D*>(iter->second->getObject());
      result.set(Quality::Good);
      result.addMetadata("CheckRZVertexZDisplacedBad", "good");
      TH1D* projectZ = h->ProjectionY();
      if ((projectZ->Integral(1, projectZ->FindBin(-10)) > 0) || (projectZ->Integral(projectZ->FindBin(10), projectZ->GetNbinsX()) > 0)) {
        result.updateMetadata("CheckRZVertexZDisplacedBad", "bad");
        result.set(Quality::Bad);
      }
    }

    if (iter->second->getName() == "VertexZ") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      result.set(Quality::Good);
      result.addMetadata("CheckZVertexEmpty", "good");
      result.addMetadata("CheckZVertexMeanMed", "good");
      result.addMetadata("CheckZVertexMeanBad", "good");
      result.addMetadata("CheckZVertexRms", "good");
      if (std::abs(h->GetMean()) > 1. && std::abs(h->GetMean()) < 3.) {
        result.updateMetadata("CheckZVertexMeanMed", "medium");
        result.set(Quality::Medium);
      }
      if (h->GetRMS() >= 7.) {
        result.updateMetadata("CheckZVertexRms", "medium");
        result.set(Quality::Medium);
      }
      if (h->GetEntries() < 1e-15 || h->Integral(h->FindBin(0) + 1, h->FindBin(10)) < 1e-15) {
        result.updateMetadata("CheckZVertexEmpty", "bad");
        result.set(Quality::Bad);
      }
      if (std::abs(h->GetMean()) >= 3.) {
        result.updateMetadata("CheckZVertexMeanBad", "bad");
        result.set(Quality::Bad);
      }
    }
  }
  return result;
}

std::string ITSTrackCheck::getAcceptedType() { return "TH1D"; }

void ITSTrackCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::vector<string> vPlotWithTextMessage = convertToArray<string>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "plotWithTextMessage", ""));
  std::vector<string> vTextMessage = convertToArray<string>(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "textMessage", ""));
  std::map<string, string> ShifterInfoText;

  if ((int)vTextMessage.size() == (int)vPlotWithTextMessage.size()) {
    for (int i = 0; i < (int)vTextMessage.size(); i++) {
      ShifterInfoText[vPlotWithTextMessage[i]] = vTextMessage[i];
    }
  } else
    ILOG(Warning, Support) << "Bad list of plot with TextMessages for shifter, check .json" << ENDM;

  std::shared_ptr<TLatex> tShifterInfo = std::make_shared<TLatex>(0.005, 0.006, Form("#bf{%s}", TString(ShifterInfoText[mo->getName()]).Data()));
  tShifterInfo->SetTextSize(0.04);
  tShifterInfo->SetTextFont(43);
  tShifterInfo->SetNDC();

  TString text[2];
  TString status;
  int textColor;

  if (mo->getName() == "NClusters") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {

      if (checkResult == Quality::Bad) {
        status = "Quality::Bad (call expert)";
        textColor = kRed;
      } else {
        status = "Quality::Medium";
        textColor = kOrange;
      }

      if (strcmp(checkResult.getMetadata("CheckTracks4").c_str(), "bad") == 0) {
        tMessage[0] = std::make_shared<TLatex>(0.12, 0.6, "0 tracks with 4 clusters (OK if it's synthetic run)");
        tMessage[0]->SetTextFont(43);
        tMessage[0]->SetTextSize(0.04);
        tMessage[0]->SetTextColor(kRed);
        tMessage[0]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[0]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckTracks5").c_str(), "bad") == 0) {
        tMessage[1] = std::make_shared<TLatex>(0.12, 0.55, "0 tracks with 5 clusters (OK if it's synthetic run)");
        tMessage[1]->SetTextFont(43);
        tMessage[1]->SetTextSize(0.04);
        tMessage[1]->SetTextColor(kRed);
        tMessage[1]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[1]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckTracks6").c_str(), "bad") == 0) {
        tMessage[2] = std::make_shared<TLatex>(0.12, 0.5, "0 tracks with 6 clusters (OK if it's synthetic run)");
        tMessage[2]->SetTextFont(43);
        tMessage[2]->SetTextSize(0.04);
        tMessage[2]->SetTextColor(kRed);
        tMessage[2]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[2]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckTracks7").c_str(), "bad") == 0) {
        tMessage[3] = std::make_shared<TLatex>(0.12, 0.45, "0 tracks with 7 clusters (call!)");
        tMessage[3]->SetTextFont(43);
        tMessage[3]->SetTextSize(0.04);
        tMessage[3]->SetTextColor(kRed);
        tMessage[3]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[3]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckEmpty").c_str(), "bad") == 0) {
        tMessage[4] = std::make_shared<TLatex>(0.12, 0.4, "NO ITS TRACKS");
        tMessage[4]->SetTextFont(43);
        tMessage[4]->SetTextSize(0.04);
        tMessage[4]->SetTextColor(kRed);
        tMessage[4]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[4]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckMean").c_str(), "medium") == 0) {
        tMessage[5] = std::make_shared<TLatex>(0.12, 0.76, Form("#splitline{Mean (%.1f) is outside 5.2-6.2}{ignore for COSMICS and TECHNICALS}", h->GetMean()));
        tMessage[5]->SetTextFont(43);
        tMessage[5]->SetTextSize(0.04);
        tMessage[5]->SetTextColor(kOrange);
        tMessage[5]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[5]->Clone());
      }
    }

    tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo->SetTextFont(43);
    tInfo->SetTextSize(0.06);
    tInfo->SetTextColor(textColor);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());

    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }

  if (mo->getName() == "AngularDistribution") {
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {
      status = "Quality::BAD (call expert)";
      textColor = kRed;
      if (strcmp(checkResult.getMetadata("CheckAngEmpty").c_str(), "bad") == 0) {
        tMessage[0] = std::make_shared<TLatex>(0.12, 0.65, "NO ITS TRACKS");
        tMessage[0]->SetTextFont(43);
        tMessage[0]->SetTextSize(0.04);
        tMessage[0]->SetTextColor(kRed);
        tMessage[0]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[0]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckAsymmEta").c_str(), "bad") == 0) {
        tMessage[1] = std::make_shared<TLatex>(0.12, 0.6, "Asymmetric Eta distribution (OK if there are disabled ITS sectors)");
        tMessage[1]->SetTextFont(43);
        tMessage[1]->SetTextSize(0.04);
        tMessage[1]->SetTextColor(kRed);
        tMessage[1]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[1]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckAsymmPhi").c_str(), "bad") == 0) {
        tMessage[2] = std::make_shared<TLatex>(0.12, 0.55, "Asymmetric Phi distribution (OK if there are disabled ITS sectors)");
        tMessage[2]->SetTextFont(43);
        tMessage[2]->SetTextSize(0.04);
        tMessage[2]->SetTextColor(kRed);
        tMessage[2]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[2]->Clone());
      }
    }

    tInfo = std::make_shared<TLatex>(0.05, 0.95, Form("#bf{%s}", status.Data()));
    tInfo->SetTextFont(43);
    tInfo->SetTextSize(0.06);
    tInfo->SetTextColor(textColor);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }

  if (mo->getName() == "VertexCoordinates") {
    Double_t positionX, positionY;
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality::GOOD";
      textColor = kGreen;
    } else {
      if (strcmp(checkResult.getMetadata("CheckXVertexMean").c_str(), "medium") == 0) {
        status = "Quality::Medium (do not call, inform expert on MM if run is ongoing since >5min)";
        textColor = kOrange;
        tMessage[0] = std::make_shared<TLatex>(0.12, 0.65, Form("X mean is %.4f", h->GetMean(1)));
        tMessage[0]->SetTextFont(43);
        tMessage[0]->SetTextSize(0.04);
        tMessage[0]->SetTextColor(kOrange);
        tMessage[0]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[0]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckYVertexMean").c_str(), "medium") == 0) {
        status = "Quality::Medium (do not call, inform expert on MM if run is ongoing since >5min)";
        textColor = kOrange;
        tMessage[1] = std::make_shared<TLatex>(0.12, 0.6, Form("Y mean is %.4f", h->GetMean(2)));
        tMessage[1]->SetTextFont(43);
        tMessage[1]->SetTextSize(0.04);
        tMessage[1]->SetTextColor(kOrange);
        tMessage[1]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[1]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckXVertexRms").c_str(), "medium") == 0) {
        status = "Quality::Medium (do not call, inform expert on MM if run is ongoing since >5min)";
        textColor = kOrange;
        tMessage[2] = std::make_shared<TLatex>(0.12, 0.55, Form("X RMS is %.4f", h->GetRMS(1)));
        tMessage[2]->SetTextFont(43);
        tMessage[2]->SetTextSize(0.04);
        tMessage[2]->SetTextColor(kOrange);
        tMessage[2]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[2]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckYVertexRms").c_str(), "medium") == 0) {
        status = "Quality::Medium (do not call, inform expert on MM if run is ongoing since >5min)";
        textColor = kOrange;
        tMessage[3] = std::make_shared<TLatex>(0.12, 0.5, Form("Y RMS is %.4f", h->GetRMS(2)));
        tMessage[3]->SetTextFont(43);
        tMessage[3]->SetTextSize(0.04);
        tMessage[3]->SetTextColor(kOrange);
        tMessage[3]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[3]->Clone());
      }

      if (strcmp(checkResult.getMetadata("CheckXYVertexEmpty").c_str(), "bad") == 0) {
        status = "Quality::Bad (call expert)";
        textColor = kRed;
        tMessage[4] = std::make_shared<TLatex>(0.12, 0.45, "NO/BAD VERTICES (OK if run is COSMICS or if run has JUST(<5min) started)");
        tMessage[4]->SetTextFont(43);
        tMessage[4]->SetTextSize(0.04);
        tMessage[4]->SetTextColor(kRed);
        tMessage[4]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[4]->Clone());
      }
    }

    tInfo = std::make_shared<TLatex>(0.12, 0.75, Form("#bf{%s}", status.Data()));
    tInfo->SetTextFont(43);
    tInfo->SetTextSize(0.04);
    tInfo->SetTextColor(textColor);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }

  if (mo->getName() == "VertexRvsZ") {
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality:GOOD";
      textColor = kGreen;
    } else {
      status = "Quality::Bad (call expert)";
      textColor = kRed;
      if (strcmp(checkResult.getMetadata("CheckRZVertexZDisplacedBad").c_str(), "bad") == 0) {
        tMessage[0] = std::make_shared<TLatex>(0.12, 0.65, Form("INFO: vertex Z displaced > 10 cm"));
        tMessage[0]->SetTextFont(43);
        tMessage[0]->SetTextSize(0.04);
        tMessage[0]->SetTextColor(kRed);
        tMessage[0]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[0]->Clone());
      }
    }

    tInfo = std::make_shared<TLatex>(0.12, 0.75, Form("#bf{%s}", status.Data()));
    tInfo->SetTextFont(43);
    tInfo->SetTextSize(0.04);
    tInfo->SetTextColor(textColor);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }

  if (mo->getName() == "VertexZ") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      status = "Quality:GOOD";
      textColor = kGreen;
    } else {
      if (strcmp(checkResult.getMetadata("CheckZVertexMeanMed").c_str(), "medium") == 0) {
        status = "Quality::Medium (do not call, inform expert on MM if run is ongoing since >5min)";
        textColor = kOrange;
        tMessage[0] = std::make_shared<TLatex>(0.12, 0.65, Form("Z mean is %.4f", h->GetMean(1)));
        tMessage[0]->SetTextFont(43);
        tMessage[0]->SetTextSize(0.04);
        tMessage[0]->SetTextColor(kOrange);
        tMessage[0]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[0]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckZVertexRms").c_str(), "medium") == 0) {
        status = "Quality::Medium (do not call, inform expert on MM if run is ongoing since >5min)";
        textColor = kOrange;
        tMessage[1] = std::make_shared<TLatex>(0.12, 0.6, Form("Z RMS is %.4f", h->GetRMS(1)));
        tMessage[1]->SetTextFont(43);
        tMessage[1]->SetTextSize(0.04);
        tMessage[1]->SetTextColor(kOrange);
        tMessage[1]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[1]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckZVertexMeanBad").c_str(), "bad") == 0) {
        status = "Quality::Bad (call expert)";
        textColor = kRed;
        tMessage[2] = std::make_shared<TLatex>(0.12, 0.55, Form("Z Mean is %.4f (OK if run has JUST(<5min) started)", h->GetMean(1)));
        tMessage[2]->SetTextFont(43);
        tMessage[2]->SetTextSize(0.04);
        tMessage[2]->SetTextColor(kRed);
        tMessage[2]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[2]->Clone());
      }
      if (strcmp(checkResult.getMetadata("CheckZVertexEmpty").c_str(), "bad") == 0) {
        status = "Quality::Bad (call expert)";
        textColor = kRed;
        tMessage[3] = std::make_shared<TLatex>(0.12, 0.45, "NO/BAD VERTICES (OK if run is COSMICS or if run has JUST(<5min) started)");
        tMessage[3]->SetTextFont(43);
        tMessage[3]->SetTextSize(0.04);
        tMessage[3]->SetTextColor(kRed);
        tMessage[3]->SetNDC();
        h->GetListOfFunctions()->Add(tMessage[3]->Clone());
      }
    }

    tInfo = std::make_shared<TLatex>(0.12, 0.75, Form("#bf{%s}", status.Data()));
    tInfo->SetTextFont(43);
    tInfo->SetTextSize(0.04);
    tInfo->SetTextColor(textColor);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo->Clone());
    if (ShifterInfoText[mo->getName()] != "")
      h->GetListOfFunctions()->Add(tShifterInfo->Clone());
  }
}

int ITSTrackCheck::getDigit(int number, int digit)
{
  return number % (int)pow(10, digit) / (int)pow(10, digit - 1);
}

} // namespace o2::quality_control_modules::its
