// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   RawQcCheck.cxx
/// \author My Name
///

#include "MID/RawQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>

using namespace std;

namespace o2::quality_control_modules::mid
{

void RawQcCheck::configure(std::string) {}

Quality RawQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  int nDigMT11, nDigMT12, nDigMT21, nDigMT22;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "mDetElemID") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;

        nDigMT11 = nDigMT12 = nDigMT21 = nDigMT22 = 0;
      // count digits in stations
      for (int i = 1; i <= h->GetNbinsX(); i++) {
        if (i >= 1 && i <= 18) {
	  nDigMT11 += h->GetBinContent(i);
	}
        else if (i >= 19 && i <= 36) {
	  nDigMT12 += h->GetBinContent(i);
	}
        else if (i >= 37 && i <= 54) {
	  nDigMT21 += h->GetBinContent(i);
	}
        if (i >= 55 && i <= 72) {
	  nDigMT22 += h->GetBinContent(i);
	}
      }
      // set check quality
      if (nDigMT11 == 0) {
	result = Quality::Medium;
	if (nDigMT12 == 0) {
	  result = Quality::Bad;
	}
      }
      if (nDigMT22 == 0) {
	result = Quality::Medium;
	if (nDigMT21 == 0) {
	  result = Quality::Bad;
	}
      }
      
    } // end check mDetElemID
   
  }
  return result;
}

std::string RawQcCheck::getAcceptedType() { return "TH1"; }

void RawQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Info) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::mid
