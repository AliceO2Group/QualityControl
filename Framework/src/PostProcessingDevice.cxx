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
/// \file   PostProcessingDevice.cxx
/// \author Piotr Konopka
///

#include "QualityControl/PostProcessingDevice.h"

#include "QualityControl/PostProcessingRunner.h"
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/PostProcessingRunnerConfig.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DataHeaderHelpers.h"
#include "QualityControl/UserInputOutput.h"
#include "QualityControl/runnerUtils.h"

#include <Common/Exceptions.h>
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>
#include <Framework/InitContext.h>
#include <Framework/ConfigParamRegistry.h>

using namespace AliceO2::Common;
using namespace o2::framework;

namespace o2::quality_control::postprocessing
{

PostProcessingDevice::PostProcessingDevice(const PostProcessingRunnerConfig& runnerConfig)
  : mRunner(std::make_unique<PostProcessingRunner>(runnerConfig.id)),
    mDeviceName(createPostProcessingDeviceName(runnerConfig.taskName, runnerConfig.detectorName)),
    mRunnerConfig(runnerConfig)
{
}

void PostProcessingDevice::init(framework::InitContext& ctx)
{
  core::initInfologger(ctx, mRunnerConfig.infologgerDiscardParameters, ("post/" + mRunnerConfig.taskName).substr(0, core::QcInfoLogger::maxFacilityLength), mRunnerConfig.detectorName);

  if (ctx.options().isSet("configKeyValues")) {
    mRunnerConfig.configKeyValues = ctx.options().get<std::string>("configKeyValues");
  }

  // todo: read the updated config from ctx, one available
  mRunner->init(mRunnerConfig, PostProcessingConfig{ mRunner->getID(), mRunnerConfig.configTree });

  // registering state machine callbacks
  ctx.services().get<CallbackService>().set<CallbackService::Id::Start>([this, services = ctx.services()]() mutable { start(services); });
  ctx.services().get<CallbackService>().set<CallbackService::Id::Reset>([this]() { reset(); });
  ctx.services().get<CallbackService>().set<CallbackService::Id::Stop>([this, services = ctx.services()]() mutable { stop(services); });
}

void PostProcessingDevice::run(framework::ProcessingContext& ctx)
{
  // we set the publication callback each time, because we cannot be sure that
  // the reference to DataAllocator does not change
  mRunner->setPublicationCallback(publishToDPL(ctx.outputs(), mRunnerConfig.taskName));

  // When run returns false, it has done its processing.
  if (!mRunner->run()) {
    ctx.services().get<ControlService>().endOfStream();
    ctx.services().get<ControlService>().readyToQuit(QuitRequest::Me);
  }
}

std::string PostProcessingDevice::createPostProcessingDeviceName(const std::string& taskName, const std::string& detectorName)
{
  return "qc-pp-" + detectorName + "-" + taskName;
}

void PostProcessingDevice::start(ServiceRegistryRef services)
{
  mRunner->start(services);
}

void PostProcessingDevice::stop(ServiceRegistryRef services)
{
  mRunner->stop(services);
}

void PostProcessingDevice::reset()
{
  mRunner->reset();
}

const std::string& PostProcessingDevice::getDeviceName()
{
  return mDeviceName;
}

framework::Inputs PostProcessingDevice::getInputsSpecs()
{
  o2::header::DataDescription timerDescription;
  timerDescription.runtimeInit(std::string("T-" + mRunner->getID()).substr(0, o2::header::DataDescription::size).c_str());

  return { { "timer-pp-" + mRunner->getID(),
             createDataOrigin(core::DataSourceType::PostProcessingTask, mRunnerConfig.detectorName),
             timerDescription,
             0,
             Lifetime::Timer } };
}

framework::Outputs PostProcessingDevice::getOutputSpecs() const
{
  return { createUserOutputSpec(core::DataSourceType::PostProcessingTask, mRunnerConfig.detectorName, mRunnerConfig.taskName) };
}

framework::Options PostProcessingDevice::getOptions()
{
  return { { "period-timer-pp-" + mRunner->getID(), framework::VariantType::Int, static_cast<int>(mRunnerConfig.periodSeconds * 1000000), { "PP task timer period" } } };
}

} // namespace o2::quality_control::postprocessing
