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
/// \file   BcCheck.cxx
/// \author Dawid Skora dawid.mateusz.skora@cern.ch
///

#include "FV0/BcCheck.h"
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

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fv0
{

constexpr int kBinSwOnly = 1;
constexpr int kBinTcmOnly = 2;

void BcCheck::configure()
{
  long ts = 999;
  std::map<std::string, std::string> metadata;
  std::map<std::string, std::string> headers;
  auto* lhcIf = mCcdbApi.retrieveFromTFileAny<o2::parameters::GRPLHCIFData>(mPathGrpLhcIf, metadata, ts, &headers);
  if (!lhcIf) {
    ILOG(Error, Support) << "object \"" << mPathGrpLhcIf << "\" NOT retrieved. BcCheck will not produce valid QC plots." << ENDM;
    return;
  }
  const std::string bcName = lhcIf->getInjectionScheme();
  if (bcName.size() == 8) {
    if (bcName.compare("no_value")) {
      ILOG(Error, Support) << "Filling scheme not set. BcCheck will not produce valid QC plots." << ENDM;
    }
  } else {
    ILOG(Info, Support) << "Filling scheme: " << bcName.c_str() << ENDM;
  }
  auto bcPattern = lhcIf->getBunchFilling();
}

Quality BcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  int mNumErrors = 0;
  bool metadataFound = false;
  const std::string metadataKey = "BcVsTrgIntegralBin";

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "BCvsFEEmodules" || mo->getName() == "BCvsTriggers") {
      auto* histogram = dynamic_cast<TH2*>(mo->getObject());

      if (!histogram) {
        ILOG(Error, Support) << "check(): MO " << mo->getName() << " not found" << ENDM;
        result.addReason(FlagReasonFactory::Unknown(), "MO " + mo->getName() + " not found");
        result.set(Quality::Null);
        return result;
      }

      // TO DO Recive bcPattern from hists metadata
      float metadata_info = 0;
      for (auto metainfo : mo->getMetadataMap()) {
        if (metainfo.first == metadataKey) {
          metadata_info = std::stof(metainfo.second);
          metadataFound = true;
        }
      }
    }
  }

  return result;
}

std::string BcCheck::getAcceptedType() { return "TH2"; }

void BcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "BCvsFEEmodules" || mo->getName() == "BCvsTriggers") {
    auto* histogram = dynamic_cast<TH2*>(mo->getObject());

    if (!histogram) {
      ILOG(Error, Support) << "beautify(): MO " << mo->getName() << " not found" << ENDM;
      return;
    }

    TPaveText* msg = new TPaveText(mPositionMsgBox[0], mPositionMsgBox[1], mPositionMsgBox[2], mPositionMsgBox[3], "NDC");
    histogram->GetListOfFunctions()->Add(msg);
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    msg->AddText(checkResult.getReasons()[0].second.c_str());
    int color = kBlack;
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
    msg->SetFillStyle(1);
    msg->SetLineWidth(3);
    msg->SetLineColor(color);
    msg->SetShadowColor(color);
    msg->SetTextColor(color);
    msg->SetMargin(0.0);
  }
}

} // namespace o2::quality_control_modules::fv0
