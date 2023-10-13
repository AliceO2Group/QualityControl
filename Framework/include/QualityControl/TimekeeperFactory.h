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
/// \file   TimekeeperFactory.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_TIMEKEEPERFACTORY_H
#define QUALITYCONTROL_TIMEKEEPERFACTORY_H

#include "QualityControl/Timekeeper.h"
#include <Framework/DataTakingContext.h>
#include <memory>

namespace o2::quality_control::core
{

class TimekeeperFactory
{
 public:
  static std::unique_ptr<Timekeeper> create(framework::DeploymentMode, validity_time_t windowLengthMs = 0);
  static bool needsGRPECS(framework::DeploymentMode);
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_TIMEKEEPERFACTORY_H
