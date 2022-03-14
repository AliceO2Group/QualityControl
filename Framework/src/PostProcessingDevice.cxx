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
#include "QualityControl/PostProcessingRunnerConfig.h"
#include "QualityControl/QcInfoLogger.h"

#include <Common/Exceptions.h>
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>

using namespace AliceO2::Common;
using namespace o2::framework;

constexpr auto outputBinding = "mo";

namespace o2::quality_control::postprocessing
{

PostProcessingDevice::PostProcessingDevice(const PostProcessingRunnerConfig& runnerConfig)
  : mRunner(std::make_unique<PostProcessingRunner>(runnerConfig.taskName)),
    mDeviceName(createPostProcessingIdString() + "-" + runnerConfig.taskName),
    mRunnerConfig(runnerConfig)
{
  core::QcInfoLogger::setFacility("PostProcessing");
}

void PostProcessingDevice::init(framework::InitContext& ctx)
{
  // todo: read the updated config from ctx, one available
  mRunner->init(mRunnerConfig, PostProcessingConfig{ mRunnerConfig.taskName, mRunnerConfig.configTree });

  // registering state machine callbacks
  ctx.services().get<CallbackService>().set(CallbackService::Id::Start, [this]() { start(); });
  ctx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });
  ctx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });
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

std::string PostProcessingDevice::createPostProcessingIdString()
{
  return "PP-TASK-RUNNER";
}
o2::header::DataOrigin PostProcessingDevice::createPostProcessingDataOrigin()
{
  return header::DataOrigin{ "QC" };
}
header::DataDescription PostProcessingDevice::createPostProcessingDataDescription(const std::string& taskName)
{
  if (taskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty taskName for pp-task's data description"));
  }
  o2::header::DataDescription description;
  description.runtimeInit(std::string(taskName.substr(0, header::DataDescription::size - 3) + "-mo").c_str());
  return description;
}

void PostProcessingDevice::start()
{
  mRunner->start();
}

void PostProcessingDevice::stop()
{
  mRunner->stop();
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
  timerDescription.runtimeInit(std::string("T-" + mRunner->getName()).substr(0, o2::header::DataDescription::size).c_str());

  return { { "timer-pp-" + mRunner->getName(),
             createPostProcessingDataOrigin(),
             timerDescription,
             0,
             Lifetime::Timer } };
}

framework::Outputs PostProcessingDevice::getOutputSpecs()
{
  return { { { outputBinding }, createPostProcessingDataOrigin(), createPostProcessingDataDescription(mRunner->getName()), 0 } };
}

framework::Options PostProcessingDevice::getOptions()
{
  return { { "period-timer-pp-" + mRunner->getName(), framework::VariantType::Int, static_cast<int>(mRunnerConfig.periodSeconds * 1000000), { "PP task timer period" } } };
}

} // namespace o2::quality_control::postprocessing