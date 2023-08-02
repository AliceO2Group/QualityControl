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
/// \file   TimekeeperFactory.cxx
/// \author Piotr Konopka
///

#include "QualityControl/TimekeeperFactory.h"
#include "QualityControl/TimekeeperSynchronous.h"
#include "QualityControl/TimekeeperAsynchronous.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2::framework;

namespace o2::quality_control::core
{

std::unique_ptr<Timekeeper> TimekeeperFactory::create(framework::DeploymentMode deploymentMode)
{
  switch (deploymentMode) {
    case DeploymentMode::Grid: {
      ILOG(Info, Devel) << "Detected async deployment, object validity will be based on incoming data and available SOR/EOR times" << ENDM;
      return std::make_unique<TimekeeperAsynchronous>();
      break;
    }
    case DeploymentMode::Local:
    case DeploymentMode::OnlineECS:
    case DeploymentMode::OnlineDDS:
    case DeploymentMode::OnlineAUX:
    case DeploymentMode::FST:
    default: {
      ILOG(Info, Devel) << "Detected sync deployment, object validity will be based primarily on current time" << ENDM;
      return std::make_unique<TimekeeperSynchronous>();
    }
  }
}

bool TimekeeperFactory::needsGRPECS(framework::DeploymentMode deploymentMode)
{
  return deploymentMode == DeploymentMode::Grid;
}

} // namespace o2::quality_control::core