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
/// \file    WorkflowType.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/WorkflowType.h"
#include <Framework/ConfigParamRegistry.h>

namespace o2::quality_control::core
{

namespace workflow_type_helpers
{
WorkflowType getWorkflowType(const framework::ConfigParamRegistry& options)
{
  if (options.get<bool>("local")) {
    return WorkflowType::Local;
  } else if (options.get<bool>("remote")) {
    return WorkflowType::Remote;
  } else if (options.get<bool>("full-chain")) {
    return WorkflowType::FullChain;
  } else if (!options.get<std::string>("local-batch").empty()) {
    return WorkflowType::LocalBatch;
  } else if (!options.get<std::string>("remote-batch").empty()) {
    return WorkflowType::RemoteBatch;
  } else {
    return WorkflowType::Standalone;
  }
}
} // namespace workflow_type_helpers

} // namespace o2::quality_control::core