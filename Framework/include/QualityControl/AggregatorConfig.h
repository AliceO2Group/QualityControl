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
/// \file   AggregatorConfig.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_AGGREGATORCONFIG_H
#define QC_CORE_AGGREGATORCONFIG_H

#include <vector>
#include <string>
#include <unordered_map>

#include <Framework/DataProcessorSpec.h>
#include "QualityControl/UpdatePolicyType.h"
#include "QualityControl/AggregatorSource.h"
#include "QualityControl/UserCodeConfig.h"

namespace o2::quality_control::checker
{

/// \brief  Container for the configuration of an Aggregator.
struct AggregatorConfig : public o2::quality_control::core::UserCodeConfig {
  UpdatePolicyType policyType = UpdatePolicyType::OnAny;
  std::vector<std::string> objectNames{}; // fixme: if object names are empty, allObjects are true, consider reducing to one var // fixme: duplicates "sources"
  bool allObjects = false;
  framework::Inputs inputSpecs{};
  framework::OutputSpec qoSpec{ "XXX", "INVALID" };
  std::vector<AggregatorSource> sources;
};

} // namespace o2::quality_control::checker

#endif // QC_CORE_AGGREGATORCONFIG_H
