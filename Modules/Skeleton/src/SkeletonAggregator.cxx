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
/// \file   SkeletonCheck.cxx
/// \author My Name
///

#include "Skeleton/SkeletonAggregator.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::skeleton
{

void SkeletonAggregator::configure(std::string) {}

std::map<std::string, Quality> SkeletonAggregator::aggregate(QualityObjectsMapType& qoMap)
{
  std::map<std::string, Quality> result;

  ILOG(Info, Devel) << "Entered SkeletonAggregator::aggregate" << ENDM;
  ILOG(Info, Devel) << "   received a list of size : " << qoMap.size() << ENDM;
  for (const auto& item : qoMap) {
    ILOG(Info, Devel) << "Object: " << (*item.second) << ENDM;
  }

  // we return the worse quality of all the objects we receive
  Quality current = Quality::Good;
  for (auto qo : qoMap) {
    if (qo.second->getQuality().isWorseThan(current)) {
      current = qo.second->getQuality();
    }
  }

  ILOG(Info, Devel) << "   result: " << current << ENDM;
  result["newQuality"] = current;

  // add one more
  Quality plus = Quality::Medium;
  result["another"] = plus;

  return result;
}
} // namespace o2::quality_control_modules::skeleton
