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
#include "QualityControl/runnerUtils.h"

#include <Common/Exceptions.h>
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>
#include <Framework/InitContext.h>
#include <Framework/ConfigParamRegistry.h>

using namespace AliceO2::Common;
using namespace o2::framework;

constexpr auto outputBinding = "mo";

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
  mRunner->setPublicationCallback(publishToDPL(ctx.outputs(), outputBinding));

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

header::DataOrigin PostProcessingDevice::createPostProcessingDataOrigin(const std::string& detectorCode)
{
  // We need a unique Data Origin, so we can have PP Tasks with the same names for different detectors.
  // However, to avoid colliding with data marked as e.g. TPC/CLUSTERS, we add 'P' to the data origin, so it is P<det>.
  std::string originStr = "P";
  if (detectorCode.empty()) {
    ILOG(Warning, Support) << "empty detector code for a task data origin, trying to survive with: DET" << ENDM;
    originStr += "DET";
  } else if (detectorCode.size() > 3) {
    ILOG(Warning, Support) << "too long detector code for a task data origin: " + detectorCode + ", trying to survive with: " + detectorCode.substr(0, 3) << ENDM;
    originStr += detectorCode.substr(0, 3);
  } else {
    originStr += detectorCode;
  }
  o2::header::DataOrigin origin;
  origin.runtimeInit(originStr.c_str());
  return origin;
}

header::DataDescription PostProcessingDevice::createPostProcessingDataDescription(const std::string& taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for pp-task's data description"));
  }

  return quality_control::core::createDataDescription(taskName, PostProcessingDevice::descriptionHashLength);
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
             createPostProcessingDataOrigin(mRunnerConfig.detectorName),
             timerDescription,
             0,
             Lifetime::Timer } };
}

framework::Outputs PostProcessingDevice::getOutputSpecs() const
{
  return { { { outputBinding }, createPostProcessingDataOrigin(mRunnerConfig.detectorName), createPostProcessingDataDescription(mRunnerConfig.taskName), 0, Lifetime::Sporadic } };
}

framework::Options PostProcessingDevice::getOptions()
{
  return { { "period-timer-pp-" + mRunner->getID(), framework::VariantType::Int, static_cast<int>(mRunnerConfig.periodSeconds * 1000000), { "PP task timer period" } } };
}

} // namespace o2::quality_control::postprocessing
