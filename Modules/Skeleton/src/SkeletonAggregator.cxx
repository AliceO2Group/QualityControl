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
/// \file   SkeletonCheck.cxx
/// \author My Name
///

#include "Skeleton/SkeletonAggregator.h"

#include <iostream>

void o2::quality_control_modules::skeleton::SkeletonAggregator::configure(std::string) {}

std::vector<Quality> o2::quality_control_modules::skeleton::SkeletonAggregator::aggregate(QualityObjectsMapType& qoMap)
{
  std::vector<Quality> result;

  std::cout << "HELLO FROM SKELETON AGGREGATOR" << std::endl;

  // we return the worse quality of all the objects we receive
  Quality current = Quality::Good;
  for (auto qo : qoMap) {
    std::cout << "   quality: " << qo.second->getQuality() << std::endl;
    if (qo.second->getQuality().isWorseThan(current)) {
      current = qo.second->getQuality();
    }
  }

  std::cout << "   result: " << current << std::endl;
  result.emplace_back(current);
  return result;
}
