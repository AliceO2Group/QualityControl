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

#include "MUONCommon/TracksCheck.h"
#include <regex>
#include <TH1.h>
#include <CommonDataFormat/BunchFilling.h>
#include "QualityControl/QcInfoLogger.h"
#include "MUONCommon/Helpers.h"

namespace o2::quality_control_modules::muon
{

TracksCheck::TracksCheck()
{
}

TracksCheck::~TracksCheck() = default;

void TracksCheck::configure(string /*name*/)
{
  //   auto bf = TaskInterface::retrieveConditionAny<o2::BunchFilling>("GLO/GRP/BunchFilling");
  o2::BunchFilling* bf = nullptr;
  if (bf == nullptr) {
    ILOG(Error, Support) << "could not get filling scheme" << ENDM;
  }
  auto pattern = bf ? bf->getBCPattern() : o2::BunchFilling::createPattern("3564L");

  for (auto i = 0; i < pattern.size(); i++) {
    if (pattern.test(i)) {
      mBunchCrossings.emplace_back(i);
    }
  }
}

Quality TracksCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  return result;
}

std::string TracksCheck::getAcceptedType() { return "TH1"; }

void TracksCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::regex re("TrackBC$");

  if (std::regex_search(mo->getName(), re)) {
    TH1* h = dynamic_cast<TH1*>(mo->getObject());
    if (h == nullptr) {
      return;
    }
    markBunchCrossing(*h, mBunchCrossings);
  }
}

} // namespace o2::quality_control_modules::muon
