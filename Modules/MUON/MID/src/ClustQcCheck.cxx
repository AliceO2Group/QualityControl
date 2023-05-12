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
/// \file   ClustQcCheck.cxx
/// \author Valerie Ramillien
///

#include "MID/ClustQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2F.h>
#include <TLatex.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::mid
{
void ClustQcCheck::configure()
{ // default params ::
  if (auto param = mCustomParameters.find("NbOrbitPerTF"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :NbOrbitPerTF " << param->second << ENDM;
    mOrbTF = stof(param->second);
  }
  if (auto param = mCustomParameters.find("ClusterScale"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :ClusterScale " << param->second << ENDM;
    mClusterScale = stof(param->second);
  }
}
Quality ClustQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "NbClusterTF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mClusterTF = h->GetBinContent(1);
      // std::cout << " mClusterTF  = "<< mClusterTF << std::endl ;
    }

    float scale = 1 / (mClusterTF * scaleTime * mOrbTF); // (Hz)
    if (mo->getName() == "ClusterMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "ClusterMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "ClusterMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "ClusterMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      h2->Scale(scale);
    }

    (void)moName;
    if (mo->getName() == "example") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;

      for (int i = 0; i < h->GetNbinsX(); i++) {
        if (i > 0 && i < 8 && h->GetBinContent(i) == 0) {
          result = Quality::Bad;
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is bad because there is nothing in bin " + std::to_string(i));
          break;
        } else if ((i == 0 || i > 7) && h->GetBinContent(i) > 0) {
          result = Quality::Medium;
          result.addReason(FlagReasonFactory::Unknown(),
                           "It is medium because bin " + std::to_string(i) + " is not empty");
        }
      }
    }
  }
  return result;
}

std::string ClustQcCheck::getAcceptedType() { return "TH1"; }

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
  tl->SetTextFont(22); // Normal 42
  tl->SetTextSize(0.08);
  tl->SetTextColor(color);

  return tl;
}
void ClustQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  if (mo->getName() == "ClusterMap11") {
    auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
    updateTitle(h2, "(Hz)");
    updateTitle(h2, Form("- TF=%3.0f -", mClusterTF));
    updateTitle(h2, getCurrentTime());
    h2->SetMaximum(mClusterScale);
  }
  if (mo->getName() == "ClusterMap12") {
    auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
    updateTitle(h2, "(Hz)");
    updateTitle(h2, Form("- TF=%3.0f -", mClusterTF));
    updateTitle(h2, getCurrentTime());
    h2->SetMaximum(mClusterScale);
  }
  if (mo->getName() == "ClusterMap21") {
    auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
    updateTitle(h2, "(Hz)");
    updateTitle(h2, Form("- TF=%3.0f -", mClusterTF));
    updateTitle(h2, getCurrentTime());
    h2->SetMaximum(mClusterScale);
  }
  if (mo->getName() == "ClusterMap22") {
    auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
    updateTitle(h2, "(Hz)");
    updateTitle(h2, Form("- TF=%3.0f -", mClusterTF));
    updateTitle(h2, getCurrentTime());
    h2->SetMaximum(mClusterScale);
  }

  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::mid
