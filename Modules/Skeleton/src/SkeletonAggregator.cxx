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
void o2::quality_control_modules::skeleton::SkeletonAggregator::configure(std::string){}

std::vector<Quality> o2::quality_control_modules::skeleton::SkeletonAggregator::aggregate(QualityObjectsType* qos)
{
  std::vector<Quality> result;

  // we return the worse quality of all the objects we receive
  Quality current = Quality::Good;
  for(auto qo : *qos) {
    if(qo->getQuality().isWorseThan(current)) {
      current = qo->getQuality();
    }
  }

  result.emplace_back(current);
  return result;
}
