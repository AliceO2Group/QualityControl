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
/// \file   CalibMQcCheck.cxx
/// \author Valerie Ramillien
///

#include "MID/CalibMQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TStyle.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TLatex.h>
#include <TPaveText.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::mid
{

Quality CalibMQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // printf("\n*********** CalibMQcCheck ****** check \n");
  Quality result = Quality::Null;
  mIsEmpty = 0;
  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (!mIsEmpty && mo->getName() == "MBendBadMap11") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!h2->GetEntries())
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
    if (!mIsEmpty && mo->getName() == "MBendBadMap12") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!h2->GetEntries())
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
    if (!mIsEmpty && mo->getName() == "MBendBadMap21") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!h2->GetEntries())
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
    if (!mIsEmpty && mo->getName() == "MBendBadMap22") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!h2->GetEntries())
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
    if (!mIsEmpty && mo->getName() == "MNBendBadMap11") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!h2->GetEntries())
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
    if (!mIsEmpty && mo->getName() == "MNBendBadMap12") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!h2->GetEntries())
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
    if (!mIsEmpty && mo->getName() == "MNBendBadMap21") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!h2->GetEntries())
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
    if (!mIsEmpty && mo->getName() == "MNBendBadMap22") {
      auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
      if (!(h2->GetEntries()))
        mIsEmpty = 1;
      // h2->Scale(scale);
    }
  }
  return result;
}

std::string CalibMQcCheck::getAcceptedType() { return "TH1"; }

static void updateTitle(TH1* hist, std::string suffix)
{
  if (!hist) {
    return;
  }
  TString title = hist->GetTitle();
  title.Append(" (Hz) ");
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

void CalibMQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // printf("\n*********** CalibMQcCheck ****** beautify \n");
  gStyle->SetPalette(kRainBow);
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH2I*>(mo->getObject()), currentTime);
  TLatex* msg;
  if (mo->getName() == "MBendBadMap11") {
    auto* h2 = dynamic_cast<TH2I*>(mo->getObject());
    if (mIsEmpty == 1) {
      msg = drawLatex(.2, 0.8, kRed, " EMPTY ");
      h2->GetListOfFunctions()->Add(msg);
      msg = drawLatex(.1, 0.65, kRed, " Calib Objets not produced ");
      h2->GetListOfFunctions()->Add(msg);
      msg = drawLatex(.15, 0.5, kRed, " Quality::BAD ");
      h2->GetListOfFunctions()->Add(msg);
    } else {
      msg = drawLatex(.2, 0.8, kGreen, "  Not Empty ");
      h2->GetListOfFunctions()->Add(msg);
      // msg = drawLatex(.15, 0.65, kGreen, " Calib Objets   ");
      // h2->GetListOfFunctions()->Add(msg);
      msg = drawLatex(.15, 0.5, kGreen, " Quality::GOOD ");
      h2->GetListOfFunctions()->Add(msg);
    }
  }
}

} // namespace o2::quality_control_modules::mid
