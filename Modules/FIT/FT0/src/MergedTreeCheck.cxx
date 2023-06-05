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

Quality MergedTreeCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  for (auto [name, obj] : *moMap) {

    (void)name;
    if (obj->getName() == "ChargeHistogram") {
      TH1* histogram = dynamic_cast<TH1*>(obj->getObject());
      if (histogram == nullptr) {
        ILOG(Warning, Devel) << "Could not cast " << obj->getName() << " to TH1*, skipping" << ENDM;
        continue;
      }
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
