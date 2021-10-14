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
/// \file   CheckDRMDiagnostics.cxx
/// \author Nicolo' Jacazio, Pranjal Sarma
/// \brief  Checker dedicated to the study of low level raw data diagnostics words
///

// QC
#include "TOF/CheckDRMDiagnostics.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckDRMDiagnostics::configure(std::string)
{
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckDRMDiagnostics::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking quality of diagnostic words";

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "DRMCounter") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
        continue;
      }
      for (int i = 2; i < h->GetNbinsX(); i++) {
        for (int j = 2; j < h->GetNbinsY(); j++) {
          if (h->GetBinContent(i, j) > 0) { // If larger than zero
            result = Quality::Bad;
            break;
          }
        }
        if (result == Quality::Bad) {
          break;
        }
      }
      result = Quality::Good;
    }
  }
  return result;
}

std::string CheckDRMDiagnostics::getAcceptedType() { return "TH2F"; }

void CheckDRMDiagnostics::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "DRMCounter") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    if (!h) {
      ILOG(Warning, Support) << "Did not get MO for DRMCounter";
      return;
    }
    TPaveText* msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (checkResult == Quality::Good) {
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("DRM reporting error words");
    } else if (checkResult == Quality::Medium) {
      msg->AddText("No entries. IF TOF IN RUN");
      msg->AddText("email TOF on-call.");
    }
  } else
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
}

} // namespace o2::quality_control_modules::tof
