// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   AdvancedWorkflow.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_AdvancedWorkflow_H
#define QUALITYCONTROL_AdvancedWorkflow_H

#include <Framework/WorkflowSpec.h>

/// These methods can be used to build a complex processing topology. It spawns 3 separate dummy processing chains.

namespace o2::quality_control::core
{
o2::framework::WorkflowSpec getProcessingTopology(o2::framework::DataAllocator::SubSpecificationType subspec);
o2::framework::WorkflowSpec getFullProcessingTopology();
} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_AdvancedWorkflow_H
