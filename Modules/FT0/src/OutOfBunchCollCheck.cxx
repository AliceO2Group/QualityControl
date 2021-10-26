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
/// \file   OutOfBunchCollCheck.cxx
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#include "FT0/OutOfBunchCollCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::ft0
{

void OutOfBunchCollCheck::configure(std::string)
{
  if (auto param = mCustomParameters.find("thresholdWarning"); param != mCustomParameters.end()) {
    mThreshWarning = stof(param->second);
    ILOG(Info, Support) << "configure() : using thresholdWarning = " << mThreshWarning << ENDM;
  } else {
    mThreshWarning = 1e-3;
    ILOG(Info, Support) << "configure() : using default thresholdWarning = " << mThreshWarning << ENDM;
  }

  if (auto param = mCustomParameters.find("thresholdError"); param != mCustomParameters.end()) {
    mThreshError = stof(param->second);
    ILOG(Info, Support) << "configure() : using thresholdError = " << mThreshError << ENDM;
  } else {
    mThreshError = 0.1;
    ILOG(Info, Support) << "configure() : using default thresholdError = " << mThreshError << ENDM;
  }
}

Quality OutOfBunchCollCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  TH2F* hOutOfBunchColl = 0;
  float countsBcOrbitMap = 0;
  float countsOutOfBunchColl = 0;
  bool metadataFound = false;
  std::string metadataKey = "BcOrbitMapIntegral";

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName().find("OutOfBunchColl") != std::string::npos) {
      hOutOfBunchColl = dynamic_cast<TH2F*>(mo->getObject());
      for (auto metainfo : mo->getMetadataMap()) {
        if (metainfo.first == metadataKey) {
          countsBcOrbitMap = std::stof(metainfo.second);
          metadataFound = true;
        }
      }
    }
  }
  std::string reason = "";
  if (!countsBcOrbitMap)
    reason = Form("Cannot compute quality due to zero counts in BcOrbitMap");
  if (!metadataFound)
    reason = Form("Cannot compute quality due to missing metadata: %s", metadataKey.c_str());
  if (!hOutOfBunchColl)
    reason = Form("Cannot compute quality due to problem with retieving MO");
  if (reason != "") {
    result.set(Quality::Null);
    result.addReason(FlagReasonFactory::Unknown(), reason);
    ILOG(Warning) << reason << ENDM;
    return result;
  }

  result = Quality::Good;
  mFractionOutOfBunchColl = 0;
  mNumNonEmptyBins = 0;

  countsOutOfBunchColl = hOutOfBunchColl->Integral();
  mFractionOutOfBunchColl = countsOutOfBunchColl / countsBcOrbitMap;
  if (mFractionOutOfBunchColl > mThreshError) {
    result.set(Quality::Bad);
    result.addReason(FlagReasonFactory::Unknown(),
                     Form("fraction of out of bunch collisions (%.2e) is above \"Error\" threshold (%.2e)",
                          mFractionOutOfBunchColl, mThreshError));
  } else if (mFractionOutOfBunchColl > mThreshWarning) {
    result.set(Quality::Medium);
    result.addReason(FlagReasonFactory::Unknown(),
                     Form("fraction of out of bunch collisions (%.2e) is above \"Warning\" threshold (%.2e)",
                          mFractionOutOfBunchColl, mThreshWarning));
  }

  for (int i = 1; i < hOutOfBunchColl->GetNbinsX() + 1; i++) {
    for (int j = 1; j < hOutOfBunchColl->GetNbinsY() + 1; j++) {
      if (hOutOfBunchColl->GetBinContent(i, j))
        mNumNonEmptyBins++;
    }
  }
  result.addMetadata("numNonEmptyBins", std::to_string(mNumNonEmptyBins));
  return result;
}

std::string OutOfBunchCollCheck::getAcceptedType() { return "TH2"; }

void OutOfBunchCollCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* h = dynamic_cast<TH2F*>(mo->getObject());

  TPaveText* msg = new TPaveText(0.1, 0.9, 0.9, 0.95, "NDC");
  h->GetListOfFunctions()->Add(msg);
  msg->SetName(Form("%s_msg", mo->GetName()));
  msg->Clear();
  msg->SetTextAlign(12);
  std::string prefix = Form("Fraction of out of bunch collisions = %.2e  (Warning > %.2e, Error > %.2e)    ", mFractionOutOfBunchColl, mThreshWarning, mThreshError);

  if (checkResult == Quality::Good) {
    msg->SetFillColor(kGreen);
    msg->AddText((prefix + ">> Quality::Good <<").c_str());
  } else if (checkResult == Quality::Bad) {
    msg->SetFillColor(kRed);
    msg->AddText((prefix + ">> Quality::Bad <<").c_str());
  } else if (checkResult == Quality::Medium) {
    msg->SetFillColor(kOrange);
    msg->AddText((prefix + ">> Quality::Medium <<").c_str());
  } else if (checkResult == Quality::Null) {
    msg->SetFillColor(kGray);
    msg->AddText((prefix + ">> Quality::Null <<").c_str());
  }
}

} // namespace o2::quality_control_modules::ft0
