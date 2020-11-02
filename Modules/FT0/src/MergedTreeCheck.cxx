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
/// \file   MergedTreeCheck.cxx
/// \author Milosz Filus
///

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include "TH1.h"
#include "TTree.h"
// Quality Control
#include "FT0/MergedTreeCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "FT0/Utilities.h"

using namespace std;

namespace o2::quality_control_modules::ft0
{

void MergedTreeCheck::configure(std::string) {}

Quality MergedTreeCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  for (auto [name, obj] : *moMap) {

    (void)name;
    if (obj->getName() == "ChargeHistogram") {
      TH1* histogram = dynamic_cast<TH1*>(obj->getObject());
      auto entries = histogram->GetEntries();
      if (entries < 1000) {
        return Quality::Bad;
      } else {
        return Quality::Good;
      }
    }
  }

  return Quality::Bad;
}

std::string MergedTreeCheck::getAcceptedType() { return "TH1"; }

void MergedTreeCheck::beautify(std::shared_ptr<MonitorObject>, Quality)
{
}

} // namespace o2::quality_control_modules::ft0
