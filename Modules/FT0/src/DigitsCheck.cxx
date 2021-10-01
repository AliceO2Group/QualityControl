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
/// \file   DigitsCheck.cxx
/// \author Milosz Filus
///

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include "TH1.h"
#include "TTree.h"
// Quality Control
#include "FT0/DigitsCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "FT0/Utilities.h"

using namespace std;

namespace o2::quality_control_modules::ft0
{

void DigitsCheck::configure(std::string) {}

Quality DigitsCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  for (auto [name, obj] : *moMap) {
    (void)name;
    if (obj->getName() == "EventTree") {
      TTree* tree = dynamic_cast<TTree*>(obj->getObject());
      if (tree->GetEntries() == 0) {
        return Quality::Bad;
      }

      EventWithChannelData event, *pEvent = &event;
      tree->SetBranchAddress("EventWithChannelData", &pEvent);
      for (unsigned int i = 0; i < tree->GetEntries(); ++i) {
        tree->GetEntry(i);
        if (event.getEventID() < 0 || event.getBC() == o2::InteractionRecord::DummyBC || event.getOrbit() == o2::InteractionRecord::DummyOrbit || event.getTimestamp() == 0 || event.getChannels().empty()) {
          return Quality::Bad;
        }
      }

      return Quality::Good;
    }
  }

  return Quality::Bad;
}

std::string DigitsCheck::getAcceptedType() { return "TTree"; }

void DigitsCheck::beautify(std::shared_ptr<MonitorObject>, Quality)
{
}

} // namespace o2::quality_control_modules::ft0
