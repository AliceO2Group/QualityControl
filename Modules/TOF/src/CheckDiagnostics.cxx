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
/// \file   CheckDiagnostics.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker dedicated to the study of low level raw data diagnostics words
///

// QC
#include "TOF/CheckDiagnostics.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckDiagnostics::configure()
{
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckDiagnostics::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking quality of diagnostic words" << ENDM;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "RDHCounter") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
        mShifterMessages.AddMessage("No entries");
      }
    }
  }
  return result;
}

void CheckDiagnostics::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "RDHCounter") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    if (checkResult == Quality::Bad) {
      msg->AddText("No TOF hits for all events.");
    }
  } else
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName() << ENDM;
}

} // namespace o2::quality_control_modules::tof
