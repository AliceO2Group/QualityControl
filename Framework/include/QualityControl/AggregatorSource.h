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
/// \file   AggregatorSource.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_AGGREGATORSOURCE_H
#define QUALITYCONTROL_AGGREGATORSOURCE_H

#include "QualityControl/DataSourceSpec.h"
#include <string>
#include <vector>

namespace o2::quality_control::checker
{

struct AggregatorSource {
  AggregatorSource() = default;
  AggregatorSource(core::DataSourceType type, std::string name) : type(type), name(std::move(name)) {}
  core::DataSourceType type{ core::DataSourceType::Invalid };
  std::string name{};
  std::vector<std::string> objects{};
};

} // namespace o2::quality_control::checker

#endif //QUALITYCONTROL_AGGREGATORSOURCE_H
