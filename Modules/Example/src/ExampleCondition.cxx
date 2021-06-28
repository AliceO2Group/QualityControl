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
/// \file   ExampleCondition.cxx
/// \author Piotr Konopka
///

#include "Example/ExampleCondition.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control_modules::example
{

void ExampleCondition::configure(const boost::property_tree::ptree& config)
{
  mThreshold = config.get<uint8_t>("threshold");
}

bool ExampleCondition::decide(const o2::framework::DataRef& dataRef)
{
  return dataRef.payload != nullptr ? dataRef.payload[0] > mThreshold : false;
}

} // namespace o2::quality_control_modules::example
