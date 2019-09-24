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

namespace o2::quality_control::postprocessing::trigger_helpers
{

/// \brief  Creates a trigger function by taking its corresponding name.
TriggerFcn TriggerFactory(std::string trigger);
/// \brief Creates a trigger function vector given trigger names
std::vector<TriggerFcn> createTriggers(const std::vector<std::string>& triggerNames);
/// \brief Executes a vector of triggers functions and returns the first trigger which is not Trigger::No
Trigger tryTrigger(std::vector<TriggerFcn>&);

} // namespace o2::quality_control::postprocessing::trigger_helpers

#endif //QUALITYCONTROL_TRIGGERHELPERS_H
