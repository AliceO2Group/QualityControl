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
/// \file   CFDEffCheck.cxx
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#include "FT0/CFDEffCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::ft0
{

void CFDEffCheck::configure()
{
  if (auto param = mCustomParameters.find("thresholdWarning"); param != mCustomParameters.end()) {
    mThreshWarning = stof(param->second);
    ILOG(Info, Support) << "configure() : using thresholdWarning = " << mThreshWarning << ENDM;
  } else {
    mThreshWarning = 0.999;
    ILOG(Info, Support) << "configure() : using default thresholdWarning = " << mThreshWarning << ENDM;
  }

  if (auto param = mCustomParameters.find("thresholdError"); param != mCustomParameters.end()) {
    mThreshError = stof(param->second);
    ILOG(Info, Support) << "configure() : using thresholdError = " << mThreshError << ENDM;
  } else {
    mThreshError = 0.9;
    ILOG(Info, Support) << "configure() : using default thresholdError = " << mThreshError << ENDM;
  }
}

Quality CFDEffCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "CFD_efficiency") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;
      mNumErrors = 0;
      mNumWarnings = 0;
      for (int i = 1; i < h->GetNbinsX() + 1; i++) {
        if (h->GetBinContent(i) < mThreshError) {
          if (result.isBetterThan(Quality::Bad))
            // result = Quality::Bad; // setting quality like this clears reasons
            result.set(Quality::Bad);
          mNumErrors++;
          // keep reasons short because they will be displayed on plot
          // long reasons make font size too small
          result.addReason(FlagReasonFactory::Unknown(),
                           "CFD eff. < \"Error\" threshold in channel " + std::to_string(i));
          // no need to check medium threshold
          // but don't `break` because we want to add other reasons
          continue;
        } else if (h->GetBinContent(i) < mThreshWarning) {
          if (result.isBetterThan(Quality::Medium))
            result.set(Quality::Medium);
          mNumWarnings++;
          result.addReason(FlagReasonFactory::Unknown(),
                           "CFD eff. < \"Warning\" threshold in channel " + std::to_string(i));
        }
      }
    }
  }
  result.addMetadata("nErrors", std::to_string(mNumErrors));
  result.addMetadata("nWarnings", std::to_string(mNumWarnings));
  return result;
}

std::string CFDEffCheck::getAcceptedType() { return "TH1"; }

void CFDEffCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "CFD_efficiency") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    TPaveText* msg = new TPaveText(0.15, 0.15, 0.85, 0.4, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    msg->AddText("CFDEffCheck");
    msg->AddText(Form("(Warning < %.6f, Error < %.6f)", mThreshWarning, mThreshError));

    if (checkResult == Quality::Good) {
      msg->AddText(">> Quality::Good <<");
      msg->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      auto reasons = checkResult.getReasons();
      msg->SetFillColor(kRed);
      msg->SetY2(std::min(0.7, 0.4 + reasons.size() * 0.01));
      msg->AddText(">> Quality::Bad <<");
      msg->AddText(("N channels with errors = " + checkResult.getMetadata("nErrors")).c_str());
      msg->AddText(("N channels with warnings = " + checkResult.getMetadata("nWarnings")).c_str());
      for (int i = 0; i < int(reasons.size()); i++)
        msg->AddText((reasons[i].first.getName() + ": " + reasons[i].second).c_str());
    } else if (checkResult == Quality::Medium) {
      auto reasons = checkResult.getReasons();
      msg->SetFillColor(kOrange);
      msg->SetY2(std::min(0.7, 0.4 + reasons.size() * 0.01));
      msg->AddText(">> Quality::Medium <<");
      msg->AddText(("N channels with errors = " + checkResult.getMetadata("nErrors")).c_str());
      msg->AddText(("N channels with warnings = " + checkResult.getMetadata("nWarnings")).c_str());
      for (int i = 0; i < int(reasons.size()); i++)
        msg->AddText((reasons[i].first.getName() + ": " + reasons[i].second).c_str());
    } else if (checkResult == Quality::Null) {
      msg->AddText(">> Quality::Null <<");
      msg->SetFillColor(kGray);
    }
    // add threshold lines
    Double_t xMin = h->GetXaxis()->GetXmin();
    Double_t xMax = h->GetXaxis()->GetXmax();
    auto* lineError = new TLine(xMin, mThreshError, xMax, mThreshError);
    auto* lineWarning = new TLine(xMin, mThreshWarning, xMax, mThreshWarning);
    lineError->SetLineWidth(2);
    lineWarning->SetLineWidth(2);
    lineError->SetLineStyle(2);
    lineWarning->SetLineStyle(2);
    lineError->SetLineColor(kRed);
    lineWarning->SetLineStyle(kOrange);
    h->GetListOfFunctions()->Add(lineError);
    h->GetListOfFunctions()->Add(lineWarning);
  }
}

} // namespace o2::quality_control_modules::ft0
