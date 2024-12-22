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
/// \file   ZDCRecBeautifyPlots.cxx
/// \author My Name
///

#include "ZDC/ZDCRecBeautifyPlots.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>
#include <TMarker.h>

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::zdc
{

void ZDCRecBeautifyPlots::configure()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  // This method is called whenever CustomParameters are set.

  // Example of retrieving a custom parameter
  std::string parameter = mCustomParameters.atOrDefaultValue("myOwnKey1", "default");
}

Quality ZDCRecBeautifyPlots::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  return result;
}

std::string ZDCRecBeautifyPlots::getAcceptedType()
{
  // This method is a remnant of early interface prototype and will be removed in the scope of ticket QC-373
  return "TH1";
}

void ZDCRecBeautifyPlots::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "h_CENTR_ZNA" || mo->getName() == "h_CENTR_ZNC" || mo->getName() == "h_CENTR_ZNA_cut_ZEM" || mo->getName() == "h_CENTR_ZNC_cut_ZEM") {
    auto* h = dynamic_cast<TH2F*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "could not cast '" << mo->getName() << "' to TH2*" << ENDM;
      return;
    }
    auto* lineH = new TLine(0.5, 0, -0.5, 0);
    auto* lineV = new TLine(0, 0.5, 0, -0.5);
    auto* marker = new TMarker(h->GetMean(1),h->GetMean(2),20);
    lineH->SetLineColor(kBlack);
    lineV->SetLineColor(kBlack);
    lineH->SetLineWidth(2);
    lineV->SetLineWidth(2);
    marker->SetMarkerColor(2);
    h->GetListOfFunctions()->Add(lineH);
    h->GetListOfFunctions()->Add(lineV);
    h->GetListOfFunctions()->Add(marker);
  }
}

void ZDCRecBeautifyPlots::reset()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "ZDCRecBeautifyPlots::reset" << ENDM;
  // please reset the state of the check here to allow for reuse between consecutive runs.
}

void ZDCRecBeautifyPlots::startOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "ZDCRecBeautifyPlots::start : " << activity.mId << ENDM;
}

void ZDCRecBeautifyPlots::endOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "ZDCRecBeautifyPlots::end : " << activity.mId << ENDM;
}

} // namespace o2::quality_control_modules::zdc
