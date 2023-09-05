// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QUALITYCONTROL_INFRASTRUCTURESPEC_H
#define QUALITYCONTROL_INFRASTRUCTURESPEC_H

///
/// \file   InfrastructureSpec.h
/// \author Piotr Konopka
///

#include "QualityControl/WorkflowType.h"
#include "QualityControl/CommonSpec.h"
#include "QualityControl/TaskSpec.h"
#include "QualityControl/CheckSpec.h"
#include "QualityControl/AggregatorSpec.h"
#include "QualityControl/PostProcessingTaskSpec.h"
#include "QualityControl/ExternalTaskSpec.h"

#include <vector>

namespace o2::quality_control::core
{

struct InfrastructureSpec {
  WorkflowType workflowType = WorkflowType::Standalone;
  CommonSpec common;
  std::vector<TaskSpec> tasks;
  std::vector<checker::CheckSpec> checks;
  std::vector<checker::AggregatorSpec> aggregators;
  std::vector<postprocessing::PostProcessingTaskSpec> postProcessingTasks;
  std::vector<ExternalTaskSpec> externalTasks;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_INFRASTRUCTURESPEC_H
