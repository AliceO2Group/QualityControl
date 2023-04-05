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
void CalibQcCheck::configure()
{
  ILOG(Info, Devel) << "configure CalibQcCheck" << ENDM;

  if (auto param = mCustomParameters.find("NbOrbitPerTF"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - :NbOrbitPerTF " << param->second << ENDM;
    mOrbTF = stof(param->second);
  }
}
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
      // std::cout << "mTF = "<< mTF  << std::endl;
    }
    if (mo->getName() == "NbNoiseROF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mNoiseRof = h->GetBinContent(1);
      // std::cout << "mNoiseRof = "<< mNoiseRof  << std::endl;
    }
    if (mo->getName() == "NbDeadROF") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mDeadRof = h->GetBinContent(1);
      // std::cout << "mDeadRof = "<< mDeadRof  << std::endl;
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
  // title.Append(" (kHz) ");
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
  updateTitle(dynamic_cast<TH2F*>(mo->getObject()), currentTime);
  float scaleNoise = 1.;
  float scaleDead = 1.;
  if (mOrbTF * mTF > 0)
    scaleNoise = 1. / (mTF * scaleTime * mOrbTF * 1000); // (kHz)
  if (mDeadRof > 0)
    scaleDead = 100. / mDeadRof; // % Nb dead/Nb FET

  if (checkResult == Quality::Good) {
    /// Scale Noise Maps ::
    if (mo->getName() == "BendNoiseMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10.);
    }
    if (mo->getName() == "BendNoiseMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10.);
    }
    if (mo->getName() == "BendNoiseMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10.);
    }
    if (mo->getName() == "BendNoiseMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10.);
    }
    if (mo->getName() == "NBendNoiseMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10);
    }
    if (mo->getName() == "NBendNoiseMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10.);
    }
    if (mo->getName() == "NBendNoiseMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10.);
    }
    if (mo->getName() == "NBendNoiseMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (kHz)");
      h2->Scale(scaleNoise);
      h2->SetMaximum(10.);
    }
    /// Scale Dead Maps ::
    if (mo->getName() == "BendDeadMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
    if (mo->getName() == "BendDeadMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
    if (mo->getName() == "BendDeadMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
    if (mo->getName() == "BendDeadMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
    if (mo->getName() == "NBendDeadMap11") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
    if (mo->getName() == "NBendDeadMap12") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
    if (mo->getName() == "NBendDeadMap21") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
    if (mo->getName() == "NBendDeadMap22") {
      auto* h2 = dynamic_cast<TH2F*>(mo->getObject());
      updateTitle(h2, " (%)");
      h2->Scale(scaleDead);
    }
  }
}

} // namespace o2::quality_control_modules::mid
