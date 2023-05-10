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
/// \file   TracksQcCheck.cxx
/// \author Valerie Ramillien
///

#include "MID/TracksQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TList.h>
#include <TPaveText.h>
#include <TLatex.h>
#include <TLine.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::mid
{

void TracksQcCheck::configure()
{
  ILOG(Info, Devel) << "configure TraksQcCheck" << ENDM;
  if (auto param = mCustomParameters.find("Ratio44Threshold"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - Ratio44Threshold: " << param->second << ENDM;
    mRatio44Threshold = stof(param->second);
  }
  if (auto param = mCustomParameters.find("NbOrbitPerTF"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :NbOrbitPerTF " << param->second << ENDM;
    mOrbTF = stof(param->second);
  }
  if (auto param = mCustomParameters.find("TracksScale"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :TracksScale " << param->second << ENDM;
    mTracksScale = stof(param->second);
  }
}

Quality TracksQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "NbTracksTF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mTracksTF = h->GetBinContent(1);
      // std::cout << " mTracksF  = "<< mTracksTF << std::endl ;
    }
    float scale = 1 / (mTracksTF * scaleTime * mOrbTF); // (Hz)

    if (mo->getName() == "TrackMapXY") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      h2->Scale(scale);
    }

    (void)moName;
    if (mo->getName() == "TrackRatio44") {
      auto* h = dynamic_cast<TProfile*>(mo->getObject());
      TLine* lineThreshold = new TLine(0., mRatio44Threshold, h->GetXaxis()->GetXmax(), mRatio44Threshold);
      lineThreshold->SetLineColor(kGreen);
      // lineThreshold->SetLineStyle(kDashed);
      lineThreshold->SetLineStyle(9);
      lineThreshold->SetLineWidth(3);

      result = Quality::Good;
      if (mTracksTF > 0) {
        for (int i = 1; i < h->GetNbinsX(); i++) {
          if ((i == 1) && (h->GetBinContent(i) < mRatio44Threshold)) {
            result = Quality::Bad;
            result.addReason(FlagReasonFactory::Unknown(),
                             "All Ratio44 too high in bin " + std::to_string(i));
            lineThreshold->SetLineColor(kRed);
            break;
          } else if ((i > 1) && (i < 10) && (h->GetBinContent(i) < mRatio44Threshold)) {
            result = Quality::Medium;
            result.addReason(FlagReasonFactory::Unknown(),
                             "Ratio44 too high in bin " + std::to_string(i));
            lineThreshold->SetLineColor(kOrange);
            break;
          }
        }
        h->GetListOfFunctions()->Add(lineThreshold);
      }
    } // TrackRatio44
  }
  return result;
}

std::string TracksQcCheck::getAcceptedType() { return "TH1"; }

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

static TLatex* drawLatex(double xmin, double ymin, Color_t color, TString text)
{

  TLatex* tl = new TLatex(xmin, ymin, Form("%s", text.Data()));
  tl->SetNDC();
  tl->SetTextFont(62); // Normal 42
  tl->SetTextSize(0.07);
  tl->SetTextColor(color);

  return tl;
}

void TracksQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // std::cout << "beautify ::: " << checkResult << std::endl;
  auto currentTime = getCurrentTime();
  // updateTitle(dynamic_cast<TProfile*>(mo->getObject()), currentTime);
  TLatex* msg;

  if (mo->getName() == "TrackMapXY") {
    auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
    updateTitle(h2, "(Hz)");
    updateTitle(h2, Form("- TF=%3.0f -", mTracksTF));
    updateTitle(h2, currentTime);
    h2->SetMaximum(mTracksScale);
  }
  if (mTracksTF > 0) {
    if (mo->getName() == "TrackRatio44") {
      auto* h = dynamic_cast<TProfile*>(mo->getObject());
      updateTitle(h, Form("- TF=%3.0f -", mTracksTF));
      updateTitle(h, currentTime);
      h->SetMinimum(0.);
      h->SetMaximum(1.2);

      if (checkResult == Quality::Good) {
        msg = drawLatex(.2, 0.82, kGreen, "All ratio within limits: OK!");
        msg->Draw();
        h->SetFillColor(kGreen);
        h->GetListOfFunctions()->Add(msg);
        // std::cout << "beautify GOOD::: "  << std::endl;

      } else if (checkResult == Quality::Bad) {
        ILOG(Info, Devel) << "Quality::Bad, setting to red" << ENDM;
        // msg = drawLatex(.2, 0.82, kRed, ""Global Ratio too low, call MID on-call");
        msg = drawLatex(.2, 0.82, kRed, Form("Global Ratio44/all < %4.2f  too low !! ", mRatio44Threshold));
        msg->Draw();
        h->SetFillColor(kRed);
        h->GetListOfFunctions()->Add(msg);
        // std::cout << "beautify BAD::: "  << std::endl;

      } else if (checkResult == Quality::Medium) {
        ILOG(Info, Devel) << "Quality::medium, setting to orange" << ENDM;
        msg = drawLatex(.2, 0.82, kOrange, Form("Ratio44/all < %4.2f too low !! ", mRatio44Threshold));
        msg->Draw();
        h->SetFillColor(kOrange);
        h->GetListOfFunctions()->Add(msg);
        // std::cout << "beautify MEDIUM::: "  << std::endl;
      }
      h->SetTitleSize(0.04);
      h->SetLineColor(kBlack);
    }
  }
}

} // namespace o2::quality_control_modules::mid
