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
#include <TStyle.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TLatex.h>
#include <TPaveText.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::mid
{

Quality CalibMQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // printf("\n*********** CalibMQcCheck ****** check \n");
  Quality result = Quality::Null;
  for (auto& item : *moMap) {
    if (item.second->getName() == "NbBadChannelTF") {
      auto nbTFs = static_cast<TH1F*>(item.second->getObject())->GetBinContent(1);
      mHistoHelper.setNTFs(nbTFs);
      result = (nbTFs == 0) ? Quality::Bad : Quality::Good;
      return result;
    }
  }
  return result;
}

std::string CalibMQcCheck::getAcceptedType() { return "TH1"; }

void CalibMQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  auto currentTime = mHistoHelper.getCurrentTime();
  mHistoHelper.updateTitle(dynamic_cast<TH1*>(mo->getObject()), mHistoHelper.getCurrentTime());
  if (mHistoHelper.getNTFs() == 0) {
    checkResult = Quality::Bad;
  }
  auto color = mHistoHelper.getColor(checkResult);
  if (mo->getName().find("BendBadMap") != std::string::npos) {
    auto* h2 = static_cast<TH2*>(mo->getObject());
    if (checkResult == Quality::Bad) {
      mHistoHelper.addLatex(h2, 0.2, 0.8, color, "Calib objects not produced!");
    }
    mHistoHelper.addLatex(h2, 0.15, 0.5, color, fmt::format("Quality::{}", checkResult.getName()));
  }
}

} // namespace o2::quality_control_modules::mid
