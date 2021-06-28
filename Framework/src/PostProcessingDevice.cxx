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
#include "QualityControl/QcInfoLogger.h"

#include <Common/Exceptions.h>
#include <Configuration/ConfigurationFactory.h>
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>

using namespace AliceO2::Common;
using namespace o2::configuration;
using namespace o2::framework;

constexpr auto outputBinding = "mo";

namespace o2::quality_control::postprocessing
{

PostProcessingDevice::PostProcessingDevice(const std::string& taskName, const std::string& configurationSource)
  : mRunner(std::make_unique<PostProcessingRunner>(taskName)), mDeviceName(createPostProcessingIdString() + "-" + taskName), mConfigSource(configurationSource)
{
  ILOG_INST.setFacility("PostProcessing");
}

void PostProcessingDevice::init(framework::InitContext& ctx)
{
  // todo: eventually we should retrieve the configuration from context
  auto config = ConfigurationFactory::getConfiguration(mConfigSource);
  mRunner->init(config->getRecursive());

  // registering state machine callbacks
  ctx.services().get<CallbackService>().set(CallbackService::Id::Start, [this]() { start(); });
  ctx.services().get<CallbackService>().set(CallbackService::Id::Stop, [this]() { stop(); });
  ctx.services().get<CallbackService>().set(CallbackService::Id::Reset, [this]() { reset(); });
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
  // Getting a configurable value for timer period is not straightforward at all.
  // Passing it as an exec argument would make a long route through InfrastructureGenerator and others
  // Putting it in the config file is not possible, because we need this parameter during workflow
  // creation, but PostProcessingRunner reads the config during device initialisation, not sooner.
  // It is still possible to tweak the value through AliECS, by setting a custom argument value when running this process.
  return { { "period-timer-pp-" + mRunner->getName(), framework::VariantType::Int, static_cast<int>(10 * 1000000), { "PP task timer period" } } };
}

} // namespace o2::quality_control::postprocessing