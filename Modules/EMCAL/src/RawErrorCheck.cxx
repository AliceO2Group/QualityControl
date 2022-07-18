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
/// \file   RawErrorCheck.cxx
/// \author My Name
///

#include <algorithm>
#include <array>
#include <string>

#include "EMCAL/RawErrorCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::emcal
{

void RawErrorCheck::configure() {}

Quality RawErrorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::array<std::string, 7> errorhists = { { "RawDataErrors", "PageErrors", "MajorAltroErrors", "MinorAltroError", "RawFitError", "GeometryError", "GainTypeError" } };
  std::array<std::string, 2> gainhists = { { "NoHGPerDDL", "NoLGPerDDL" } };
  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {
    if (std::find(errorhists.begin(), errorhists.end(), mo->getName()) != errorhists.end()) {
      // Check for presence of error codes
      auto* errorhist = dynamic_cast<TH2*>(mo->getObject());

      for (int linkID = 0; linkID < errorhist->GetXaxis()->GetNbins(); linkID++) {
        for (int errorcode = 0; errorcode < errorhist->GetYaxis()->GetNbins(); errorcode++) {
          if (errorhist->GetBinContent(linkID + 1, errorcode + 1)) {
            // Found raw data error
            result = Quality::Bad;
            result.addReason(FlagReasonFactory::Unknown(), "Raw error " + std::string(errorhist->GetYaxis()->GetBinLabel(errorcode + 1)) + " found in DDL " + std::to_string(linkID));
          }
        }
      }
    } else if (std::find(gainhists.begin(), gainhists.end(), mo->GetName()) != gainhists.end()) {
      // Find FEC with gain error
      auto errorhist = dynamic_cast<TH2*>(mo->getObject());
      std::string errortype;
      std::string_view histname = mo->GetName();
      if (histname == "NoHGPerDDL") {
        errortype = "LGnoHG";
      } else {
        errortype = "HGnoLG";
      }
      for (int linkID = 0; linkID < errorhist->GetXaxis()->GetNbins(); linkID++) {
        for (int fecID = 0; fecID < errorhist->GetYaxis()->GetNbins(); fecID++) {
          if (errorhist->GetBinContent(linkID + 1, fecID + 1)) {
            result = Quality::Bad;
            result.addReason(FlagReasonFactory::Unknown(), "Gain error " + errortype + " in FEC " + std::to_string(fecID) + " of DDL " + std::to_string(linkID));
          }
        }
      }
    } else {
      ILOG(Error, Support) << "Unsupported histogram in check: " << mo->GetName() << ENDM;
      continue;
    }
  }
  return result;
}

std::string RawErrorCheck::getAcceptedType() { return "TH2"; }

void RawErrorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* h = dynamic_cast<TH1*>(mo->getObject());
  if (checkResult == Quality::Good) {
    TLatex* msg = new TLatex(0.2, 0.8, "#color[418]{No Error: OK}");
    msg->SetNDC();
    msg->SetTextSize(16);
    msg->SetTextFont(43);
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
  } else if (checkResult == Quality::Bad) {
    TLatex* msg = new TLatex(0.2, 0.8, "#color[2]{Presence of Error Code: call EMCAL oncall}");
    msg->SetNDC();
    msg->SetTextSize(16);
    msg->SetTextFont(43);
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    // Notify about found errors on the infoLogger:
    for (const auto& reason : checkResult.getReasons())
      ILOG(Error, Support) << "Raw Error in " << mo->GetName() << " found: " << reason.second << ENDM;
  }
}

} // namespace o2::quality_control_modules::emcal
