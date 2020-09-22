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
/// \file    TriggerHelpers.h
/// \author  Piotr Konopka
///
#ifndef QUALITYCONTROL_TRIGGERHELPERS_H
#define QUALITYCONTROL_TRIGGERHELPERS_H

#include "QualityControl/Triggers.h"
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing::trigger_helpers
{

/// \brief  Creates a trigger function by taking its corresponding name.
TriggerFcn triggerFactory(std::string trigger, const PostProcessingConfig& config);
/// \brief Creates a trigger function vector given trigger names
std::vector<TriggerFcn> createTriggers(const std::vector<std::string>& triggerNames, const PostProcessingConfig& config);
/// \brief Executes a vector of triggers functions and returns the first trigger which is not TriggerType::No
Trigger tryTrigger(std::vector<TriggerFcn>&);
/// \brief Checks if in a given trigger configuration vector there is a UserOrControl trigger.
/// This is trigger cannot be checked as all the others, so we just check if it is requested in the right moments.
bool hasUserOrControlTrigger(const std::vector<std::string>&);

} // namespace o2::quality_control::postprocessing::trigger_helpers

#endif //QUALITYCONTROL_TRIGGERHELPERS_H
