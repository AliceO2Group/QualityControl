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
/// \file   CheckConfig.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_CHECKCONFIG_H
#define QC_CORE_CHECKCONFIG_H

#include <string>
#include <vector>

#include <Framework/DataProcessorSpec.h>
#include "QualityControl/UpdatePolicyType.h"
#include "QualityControl/UserCodeConfig.h"

namespace o2::quality_control::checker
{

/// \brief  Container for the configuration of a Check.
struct CheckConfig : public o2::quality_control::core::UserCodeConfig {
  UpdatePolicyType policyType = UpdatePolicyType::OnAny;
  std::vector<std::string> objectNames{}; // fixme: if object names are empty, allObjects are true, consider reducing to one var
  bool allObjects = false;
  bool allowBeautify = false;
  framework::Inputs inputSpecs{};
  framework::OutputSpec qoSpec{ "XXX", "INVALID" };
};

} // namespace o2::quality_control::checker

#endif // QC_CORE_CHECKCONFIG_H
