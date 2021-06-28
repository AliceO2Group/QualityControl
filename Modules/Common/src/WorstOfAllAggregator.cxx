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
/// \file   WorstOfAllAggregator.cxx
/// \author Piotr Konopka
///

#include "Common/WorstOfAllAggregator.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

void WorstOfAllAggregator::configure(std::string name)
{
  mName = name;
}

std::map<std::string, Quality> WorstOfAllAggregator::aggregate(QualityObjectsMapType& qoMap)
{
  if (qoMap.empty()) {
    Quality null = Quality::Null;
    null.addReason(FlagReasonFactory::MissingQualityObject(),
                   "QO map given to the aggregator '" + mName + "' is empty.");
    return { { mName, null } };
  }

  // we return the worse quality of all the objects we receive, but we preserve all FlagReasons
  Quality current = Quality::Good;
  for (const auto& [qoName, qo] : qoMap) {
    (void)qoName;
    for (const auto& reason : qo->getReasons()) {
      current.addReason(reason.first, reason.second);
    }
    if (qo->getQuality().isWorseThan(current)) {
      current.set(qo->getQuality());
    }
  }
  ILOG(Info, Devel) << "Aggregated Quality: " << current << ENDM;

  return { { mName, current } };
}

} // namespace o2::quality_control_modules::common
