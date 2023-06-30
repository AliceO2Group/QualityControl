// Copyright 2023 CERN and copyright holders of ALICE O2.
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
/// \file   OutOfBunchCollFeeModulesCheck.cxx
/// \author Dawid Skora dawid.mateusz.skora@cern.ch
///

#include "FV0/OutOfBunchCollFeeModulesCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsParameters/GRPLHCIFData.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TList.h>

#include <map>
#include <algorithm>
#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fv0
{

void OutOfBunchCollFeeModulesCheck::configure()
{
  if (auto param = mCustomParameters.find("thresholdWarning"); param != mCustomParameters.end()) {
    mThreshWarning = stof(param->second);
    ILOG(Debug, Support) << "configure() : using thresholdWarning = " << mThreshWarning << ENDM;
  } else {
    mThreshWarning = 1e-3;
    ILOG(Debug, Support) << "configure() : using default thresholdWarning = " << mThreshWarning << ENDM;
  }

  if (auto param = mCustomParameters.find("thresholdError"); param != mCustomParameters.end()) {
    mThreshError = stof(param->second);
    ILOG(Debug, Support) << "configure() : using thresholdError = " << mThreshError << ENDM;
  } else {
    mThreshError = 0.1;
    ILOG(Debug, Support) << "configure() : using default thresholdError = " << mThreshError << ENDM;
  }

  if (mThreshError < mThreshWarning) {
    ILOG(Debug, Support) << "thresholdError lower than thresholdWarning. Swaping values." << ENDM;
    std::swap(mThreshError, mThreshWarning);
  }
}

Quality OutOfBunchCollFeeModulesCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName().find("OutOfBunchColl_BCvsFeeModules") != std::string::npos) {
      auto* histogram = dynamic_cast<TH2*>(mo->getObject());

      if (!histogram) {
        ILOG(Error, Support) << "check(): MO " << mo->getName() << " not found" << ENDM;
        result.addReason(FlagReasonFactory::Unknown(), "MO " + mo->getName() + " not found");
        result.set(Quality::Null);
        return result;
      }

      std::vector<float> allCollPerFeeModule(mo->getMetadataMap().size() + 1, 0);
      for (auto metainfo : mo->getMetadataMap()) {
        int bin = 0;
        float value = 0;
        try {
          bin = std::stoi(metainfo.first);
          value = std::stof(metainfo.second);
          allCollPerFeeModule[bin] = value;
        } catch (const std::invalid_argument& e) {
          ILOG(Warning, Support) << "Could not get value for key " << metainfo.first << ENDM;
          continue;
        }
      }

      // Calculate out-of-bunch-coll fraction for Fee Modules
      for (int binY = 1; binY <= histogram->GetNbinsY(); binY++) {
        auto outOfBcCollisions = histogram->Integral(1, sBCperOrbit, binY, binY);
        float fraction = 0;
        if (allCollPerFeeModule[binY]) {
          fraction = outOfBcCollisions / allCollPerFeeModule[binY];
        }

        if (fraction > mFractionOutOfBunchColl) {
          mFractionOutOfBunchColl = fraction;
        }
      }

      // Check the biggest fraction of out-of-bunch-coll
      if (mFractionOutOfBunchColl > mThreshError) {
        result.set(Quality::Bad);
        result.addReason(FlagReasonFactory::Unknown(),
                         Form("Fraction of out of bunch collisions (%.2e) is above \"Error\" threshold (%.2e)",
                              mFractionOutOfBunchColl, mThreshError));
      } else if (mFractionOutOfBunchColl > mThreshWarning) {
        result.set(Quality::Medium);
        result.addReason(FlagReasonFactory::Unknown(),
                         Form("Fraction of out of bunch collisions (%.2e) is above \"Warning\" threshold (%.2e)",
                              mFractionOutOfBunchColl, mThreshWarning));
      } else {
        result.set(Quality::Good);
      }
    }
  }

  return result;
}

std::string OutOfBunchCollFeeModulesCheck::getAcceptedType() { return "TH2"; }

void OutOfBunchCollFeeModulesCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName().find("OutOfBunchColl_BCvsFeeModules") != std::string::npos) {
    auto* histogram = dynamic_cast<TH2*>(mo->getObject());

    if (!histogram) {
      ILOG(Error, Support) << "beautify(): MO " << mo->getName() << " not found" << ENDM;
      return;
    }

    TPaveText* msg = new TPaveText(0.1, 0.85, 0.9, 0.9, "NDC");
    histogram->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    if (checkResult.isWorseThan(Quality::Good)) {
      msg->AddText(checkResult.getReasons()[0].second.c_str());
    }
    int color = kWhite;
    if (checkResult == Quality::Good) {
      color = kGreen + 1;
      msg->AddText(">> Quality::Good <<");
    } else if (checkResult == Quality::Medium) {
      color = kOrange - 1;
      msg->AddText(">> Quality::Medium <<");
    } else if (checkResult == Quality::Bad) {
      color = kRed;
      msg->AddText(">> Quality::Bad <<");
    }
    msg->SetFillColor(color);
  }
}

} // namespace o2::quality_control_modules::fv0
