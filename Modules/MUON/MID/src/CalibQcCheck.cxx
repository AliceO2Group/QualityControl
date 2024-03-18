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
    ILOG(Info, Devel) << "Custom parameter - NbOrbitPerTF: " << param->second << ENDM;
    mHistoHelper.setNOrbitsPerTF(std::stol(param->second));
  }
}
Quality CalibQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // printf("\n*********** CalibQcCheck ****** check \n");
  Quality result = Quality::Null;

  for (auto& item : *moMap) {
    if (item.second->getName() == "NbTimeFrame") {
      mHistoHelper.setNTFs(dynamic_cast<TH1F*>(item.second->getObject())->GetBinContent(1));
      result = mHistoHelper.getNTFs() == 0 ? Quality::Bad : Quality::Good;
    } else if (item.second->getName() == "NbDeadROF") {
      mDeadRof = dynamic_cast<TH1F*>(item.second->getObject())->GetBinContent(1);
    }
  }
  return result;
}

std::string CalibQcCheck::getAcceptedType() { return "TH1"; }

void CalibQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto currentTime = mHistoHelper.getCurrentTime();
  mHistoHelper.updateTitle(dynamic_cast<TH2F*>(mo->getObject()), currentTime);

  if (mo->getName().find("Map") != std::string::npos || mo->getName().find("Strips") != std::string::npos) {
    // Normalize Map and Strips objects
    if (mo->getName().find("Noise") != std::string::npos) {
      // Scale histograms with noise info
      auto histo = dynamic_cast<TH1*>(mo->getObject());
      if (histo) {
	mHistoHelper.normalizeHistoTokHz(histo);
	if (mo->getName().find("Map") != std::string::npos) {
	  histo->SetMaximum(10.);
	}
      }
    } else if (mo->getName().find("Dead") != std::string::npos) {
      if (mDeadRof > 0.) {
        // Scale histograms with dead channels info
        auto histo = dynamic_cast<TH1*>(mo->getObject());
	if (histo) {	
	  histo->Scale(100. / mDeadRof);
	  mHistoHelper.updateTitle(histo, " (%)");
	}
      }
    }
  }
}
} // namespace o2::quality_control_modules::mid
