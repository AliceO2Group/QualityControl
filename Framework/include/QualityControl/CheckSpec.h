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

#ifndef QUALITYCONTROL_CHECKSPEC_H
#define QUALITYCONTROL_CHECKSPEC_H

///
/// \file   CheckSpec.h
/// \author Piotr Konopka
///

#include <string>

#include "QualityControl/DataSourceSpec.h"
#include "QualityControl/UpdatePolicyType.h"

namespace o2::quality_control::checker
{

/// \brief Specification of a Check, which should map the JSON configuration structure.
struct CheckSpec {
  // default, invalid spec
  CheckSpec() = default;

  // minimal valid spec
  CheckSpec(std::string checkName, std::string className, std::string moduleName, std::string detectorName,
            std::vector<core::DataSourceSpec> dataSources, UpdatePolicyType updatePolicySpec)
    : checkName(std::move(checkName)),
      className(std::move(className)),
      moduleName(std::move(moduleName)),
      detectorName(std::move(detectorName)),
      dataSources(std::move(dataSources)),
      updatePolicy(updatePolicySpec)
  {
  }

  // basic
  std::string checkName = "Invalid";
  std::string className = "Invalid";
  std::string moduleName = "Invalid";
  std::string detectorName = "DET";
  std::vector<core::DataSourceSpec> dataSources;
  UpdatePolicyType updatePolicy = UpdatePolicyType::OnAny;
  // advanced
  bool active = true;
  std::unordered_map<std::string, std::string> customParameters = {};
};

} // namespace o2::quality_control::checker

#endif //QUALITYCONTROL_CHECKSPEC_H
