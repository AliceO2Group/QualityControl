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
/// \file   CheckDRMDiagnostics.cxx
/// \author Nicolo' Jacazio, Pranjal Sarma
/// \brief  Checker dedicated to the study of low level raw data diagnostics words
///

// QC
#include "TOF/CheckDRMDiagnostics.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckDRMDiagnostics::configure(std::string) {}

Quality CheckDRMDiagnostics::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;
  ILOG(Info, Support) << "Checking quality of diagnostic words";

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "DRMCounter") {
      auto* h = dynamic_cast<TH2F*>(mo->getObject());
      //      if (h->GetEntries() == 0) {
      //        result = Quality::Medium;

      for (int i = 2; i < h->GetNbinsX(); i++) {
        for (int j = 2; j < h->GetNbinsY(); j++) {
          const float content = h->GetBinContent(i, j);
          if (content > 0) { // If larger than zero
            result = Quality::Bad;
          }
        }
      }
    }
  }
  return result;
}

std::string CheckDRMDiagnostics::getAcceptedType() { return "TH1F"; }

void CheckDRMDiagnostics::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  ILOG(Info, Support) << "USING BEAUTIFY";
  if (mo->getName() == "DRMCounter") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    if (!h) {
      ILOG(Warning, Support) << "Did not get MO for DRMCounter";
      return;
    }
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Bad) {
      ILOG(Info, Support) << "Quality::Bad, Error bins with content >0";
      msg->Clear();
      msg->AddText("DRM reporting error words");
      msg->SetFillColor(kGreen);
      //
      //      h->SetFillColor(kGreen);
    } else {
      ILOG(Info, Support) << "Quality::Null, No Error bins with content >0";
      msg->SetTextColor(kWhite);
      msg->SetFillColor(kBlack);
      msg->AddText("DRM words: OK!");
    }
  } else
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
}

} // namespace o2::quality_control_modules::tof
