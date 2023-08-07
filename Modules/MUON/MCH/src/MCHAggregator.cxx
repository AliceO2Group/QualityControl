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
/// \file   MCHAggregator.cxx
/// \author Andrea Ferrero
///

#include "MCH/MCHAggregator.h"

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::muonchambers
{

void MCHAggregator::configure() {}

std::map<std::string, Quality> MCHAggregator::aggregate(QualityObjectsMapType& qoMap)
{
  std::map<std::string, Quality> result;

  // we return the worse quality of all the objects we receive
  Quality current = Quality::Good;
  for (auto qo : qoMap) {
    if (qo.second->getQuality().isWorseThan(current)) {
      current = qo.second->getQuality();
    }
  }

  result["MCHQuality"] = current;

  return result;
}
} // namespace o2::quality_control_modules::muonchambers
