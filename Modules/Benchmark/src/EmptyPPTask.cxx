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
/// \file   PostProcessingInterface.cxx
/// \author My Name
///

#include "Benchmark/EmptyPPTask.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::benchmark
{

EmptyPPTask::~EmptyPPTask()
{
}

void EmptyPPTask::initialize(Trigger, framework::ServiceRegistry&)
{
}

void EmptyPPTask::update(Trigger, framework::ServiceRegistry&)
{
}

void EmptyPPTask::finalize(Trigger, framework::ServiceRegistry&)
{
}

} // namespace o2::quality_control_modules::benchmark
