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
/// \file   QualityCheck.cxx
/// \author Andrea Ferrero, Sebastien Perrin
///

#include "MCH/QualityCheck.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"

// ROOT
#include <fairlogger/Logger.h>
#include <TH1.h>
#include <TPaveText.h>
#include <TLine.h>
#include <fstream>
#include <string>

using namespace std;

namespace o2::quality_control_modules::muonchambers
{

void QualityCheck::configure()
{
  if (auto param = mCustomParameters.find("MinGoodFraction"); param != mCustomParameters.end()) {
    mMinGoodFraction = std::stof(param->second);
  }
}

Quality QualityCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  ILOG(Info, Devel) << "Entered QualityCheck::check" << ENDM;
  ILOG(Info, Devel) << "   received a list of size : " << moMap->size() << ENDM;
  for (const auto& item : *moMap) {
    ILOG(Info, Devel) << "Object: " << item.second->getName() << ENDM;
  }

  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    if (!mQualityHistName.empty() && mo->getName().find(mQualityHistName) != std::string::npos) {
      TH1F* h = dynamic_cast<TH1F*>(mo->getObject());
      if (h) {
        auto nNull = h->GetBinContent(1);
        auto nBad = h->GetBinContent(2);
        auto nGood = h->GetBinContent(3);
        auto ratio = nGood / (nGood + nBad);
        if (ratio < mMinGoodFraction) {
          result = Quality::Bad;
        } else {
          result = Quality::Good;
        }
      }
    }
  }
  return result;
}

std::string QualityCheck::getAcceptedType() { return "TH1"; }

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

void QualityCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = getCurrentTime();
  // updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  if (mo->getName().find(mQualityHistName) != std::string::npos) {
    TH1F* h = dynamic_cast<TH1F*>(mo->getObject());
    if (!h) {
      return;
    }
    TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->SetBorderSize(0);

    if (checkResult == Quality::Good) {
      msg->Clear();
      msg->AddText("All occupancies within limits: OK!!!");
      msg->SetFillColor(kGreen);

      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(info) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("Call MCH experts");
      msg->SetFillColor(kRed);

      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(info) << "Quality::medium, setting to orange";

      msg->Clear();
      msg->AddText("No entries. If MCH in the run, call MCH experts");
      msg->SetFillColor(kYellow);
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::muonchambers
