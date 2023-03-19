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
/// \file   CalibQcCheck.cxx
/// \author Valerie Ramillien
///

#include "MID/CalibQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::mid
{

Quality CalibQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // printf("\n*********** CalibQcCheck ****** check \n");
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "NbTimeFrame") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mTF = h->GetBinContent(1);
      if (mTF > 0)
        result = Quality::Good;
      else
        result = Quality::Bad;
    }
    if (mo->getName() == "NbNoiseROF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mNoiseRof = h->GetBinContent(1);
    }
    if (mo->getName() == "NbDeadROF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mDeadRof = h->GetBinContent(1);
    }
  }
  return result;
}

std::string CalibQcCheck::getAcceptedType() { return "TH1"; }

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

void CalibQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // printf("\n*********** CalibQcCheck ****** beautify \n");
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TProfile2D*>(mo->getObject()), currentTime);
  // printf("\n*********** CalibQcCheck ****** nTF = %d, nDeadRof = %d, nNoiseRof = %d\n",nTF,nDeadRof,nNoiseRof);
  if (checkResult == Quality::Good) {
    // float scale = 1 / (mTF * scaleTime); //Dead max 998,1
    // float scale = 1 / (mTF); //Dead max 11,38 (== 113826/10000TF)
    // float scale = 1.; //Dead max 113826
    float scale = 100.;
    /// Scale Noise Maps ::
    if (mo->getName() == "BendNoiseMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "BendNoiseMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "BendNoiseMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "BendNoiseMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendNoiseMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendNoiseMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendNoiseMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendNoiseMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    /// Scale Dead Maps ::
    if (mo->getName() == "BendDeadMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "BendDeadMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "BendDeadMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "BendDeadMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendDeadMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendDeadMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendDeadMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "NBendDeadMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
  }
}

} // namespace o2::quality_control_modules::mid
