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

#ifndef QUALITYCONTROL_AGGREGATORSPEC_H
#define QUALITYCONTROL_AGGREGATORSPEC_H

///
/// \file   AggregatorSpec.h
/// \author Piotr Konopka
///

#include <string>

#include "QualityControl/DataSourceSpec.h"
#include "QualityControl/UpdatePolicyType.h"
#include "QualityControl/CustomParameters.h"

namespace o2::quality_control::checker
{

/// \brief Specification of an Aggregator, which should map the JSON configuration structure.
struct AggregatorSpec {
  // default, invalid spec
  AggregatorSpec() = default;

  // minimal valid spec
  AggregatorSpec(std::string aggregatorName, std::string className, std::string moduleName, std::string detectorName,
                 std::vector<core::DataSourceSpec> dataSources, UpdatePolicyType updatePolicySpec)
    : aggregatorName(std::move(aggregatorName)),
      className(std::move(className)),
      moduleName(std::move(moduleName)),
      detectorName(std::move(detectorName)),
      dataSources(std::move(dataSources)),
      updatePolicy(updatePolicySpec)
  {
  }

  // basic
  std::string aggregatorName = "Invalid";
  std::string className = "Invalid";
  std::string moduleName = "Invalid";
  std::string detectorName = "DET";
  std::vector<core::DataSourceSpec> dataSources;
  UpdatePolicyType updatePolicy = UpdatePolicyType::OnAny;
  // advanced
  bool active = true;
  core::CustomParameters customParameters;
};

} // namespace o2::quality_control::checker
#endif //QUALITYCONTROL_AGGREGATORSPEC_H
