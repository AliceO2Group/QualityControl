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
/// \file   AggregatorInterface.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/AggregatorInterface.h"
#include "QualityControl/QCInputsAdapters.h"

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control::checker
{

std::map<std::string, core::Quality> AggregatorInterface::aggregate(std::map<std::string, std::shared_ptr<const core::QualityObject>>& qoMap)
{
  auto data = createData(qoMap);
  return aggregate(data);
}

std::map<std::string, core::Quality> AggregatorInterface::aggregate(const core::QCInputs& data)
{
  return {};
}

void AggregatorInterface::startOfActivity(const Activity& activity)
{
  // noop, override it if you want.
}

void AggregatorInterface::endOfActivity(const Activity& activity)
{
  // noop, override it if you want.
}

} // namespace o2::quality_control::checker
