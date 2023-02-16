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
#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::mid
{

Quality CalibMQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // printf("\n*********** CalibMQcCheck ****** check \n");
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "mNbTimeFrame") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mTF = h->GetBinContent(1);
      if (mTF > 0)
        result = Quality::Good;
      else
        result = Quality::Bad;
    }
    if (mo->getName() == "MNbNoiseROF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mNoiseRof = h->GetBinContent(1);
    }
    if (mo->getName() == "MNbDeadROF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mDeadRof = h->GetBinContent(1);
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

void CalibMQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // printf("\n*********** CalibMQcCheck ****** beautify \n");
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TProfile2D*>(mo->getObject()), currentTime);
  // printf("\n*********** CalibQcCheck ****** nTF = %d, nDeadRof = %d, nNoiseRof = %d\n",nTF,nDeadRof,nNoiseRof);
  if (checkResult == Quality::Good) {
    // float scale = 1 / (nTF * scaleTime); //Dead max 998,1
    // float scale = 1 / (nTF); //Dead max 11,38 (== 113826/10000TF)
    // float scale = 1.; //Dead max 113826
    float scale = 100.;
    /// Scale Noise Maps ::
    if (mo->getName() == "MBendNoiseMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MBendNoiseMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MBendNoiseMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MBendNoiseMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendNoiseMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendNoiseMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendNoiseMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendNoiseMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    /// Scale Dead Maps ::
    if (mo->getName() == "MBendDeadMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MBendDeadMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MBendDeadMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MBendDeadMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendDeadMap11") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendDeadMap12") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendDeadMap21") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
    if (mo->getName() == "MNBendDeadMap22") {
      auto* h2 = dynamic_cast<TProfile2D*>(mo->getObject());
      h2->Scale(scale);
    }
  }
}

} // namespace o2::quality_control_modules::mid
