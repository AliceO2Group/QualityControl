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
/// \file   CheckCompressedData.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw compressed data for TOF
///

// QC
#include "TOF/CheckCompressedData.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckCompressedData::configure()
{
  mDiagnosticThresholdPerSlot = 0;
  if (auto param = mCustomParameters.find("DiagnosticThresholdPerSlot"); param != mCustomParameters.end()) {
    mDiagnosticThresholdPerSlot = ::atof(param->second.c_str());
  }

  mShifterMessages.configure(0.9, 0.1, 1.0, 0.5); // Setting default before checking in configuration
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckCompressedData::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking quality of compressed data";

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "hDiagnostic") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      result = Quality::Good;
      for (int i = 1; i < h->GetNbinsX(); i++) {
        for (int j = 1; j < h->GetNbinsY(); j++) {
          const float content = h->GetBinContent(i, j);
          if (content > mDiagnosticThresholdPerSlot) { // If above threshold
            result = Quality::Bad;
          } else if (content > 0) { // If larger than zero
            result = Quality::Medium;
          }
        }
      }
    }
  }
  return result;
}

std::string CheckCompressedData::getAcceptedType() { return "TH2F"; }

void CheckCompressedData::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "hDiagnostic") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    auto msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (!msg) {
      return;
    }
    if (checkResult == Quality::Good) {
      msg->AddText("OK!");
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Diagnostics");
      msg->AddText("above");
      msg->AddText(Form("threshold (%.0f)", mDiagnosticThresholdPerSlot));
    } else if (checkResult == Quality::Medium) {
      msg->AddText("Diagnostics above zero");
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
  }
}
} // namespace o2::quality_control_modules::tof
