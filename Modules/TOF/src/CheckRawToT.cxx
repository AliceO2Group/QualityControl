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
/// \file   CheckRawToT.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for TOF Raw data on ToT
///

// QC
#include "TOF/CheckRawToT.h"
#include "QualityControl/QcInfoLogger.h"
#include "Base/MessagePad.h"

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawToT::configure(std::string)
{

  if (auto param = mCustomParameters.find("MinRawTime"); param != mCustomParameters.end()) {
    mMinRawToT = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawTime"); param != mCustomParameters.end()) {
    mMaxRawToT = ::atof(param->second.c_str());
  }
  mShifterMessages.configure(mCustomParameters);
}

Quality CheckRawToT::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto mo = moMap->begin()->second;
  Quality result = Quality::Null;

  // if ((histname.EndsWith("RawsToT")) || (histname.Contains("RawsToT") && suffixTrgCl)) {
  if (mo->getName().find("RawsToT") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
    } else {
      const float timeMean = h->GetMean();
      if ((timeMean > mMinRawToT) && (timeMean < mMaxRawToT)) {
        result = Quality::Good;
      } else {
        ILOG(Warning, Support) << Form("ToT mean = %5.2f ns", timeMean);
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string CheckRawToT::getAcceptedType() { return "TH1"; }

void CheckRawToT::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("RawsToT") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    TPaveText* msg = mShifterMessages.MakeMessagePad(h, checkResult);
    if (checkResult == Quality::Good) {
      msg->AddText("Mean inside limits: OK");
      msg->AddText(Form("Allowed range: %3.1f-%3.1f ns", mMinRawToT, mMaxRawToT));
    } else if (checkResult == Quality::Bad) {
      msg->AddText(Form("Mean outside limits (%3.1f-%3.1f ns)", mMinRawToT, mMaxRawToT));
      msg->AddText("If NOT a technical run,");
      msg->AddText("call TOF on-call.");
    } else if (checkResult == Quality::Medium) {
      msg->AddText("No entries.");
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
  }
}

} // namespace o2::quality_control_modules::tof
