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
/// \file   LateTaskRunner.cxx
/// \author Piotr Konopka
///

#include "QualityControl/LateTaskRunner.h"

// O2
#include <Common/Exceptions.h>
#include <Monitoring/MonitoringFactory.h>
#include <Monitoring/Monitoring.h>
#include <Framework/InputRecordWalker.h>
#include <CommonUtils/ConfigurableParam.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/InitContext.h>
#include <Framework/ConfigParamRegistry.h>

#include <numeric>
#include <utility>
#include <TSystem.h>

// QC
#include "QualityControl/DatabaseFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Aggregator.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/LateTaskRunnerFactory.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/ConfigParamGlo.h"
#include "QualityControl/Bookkeeping.h"
#include "QualityControl/WorkflowType.h"
#include "QualityControl/HashDataDescription.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/LateTaskFactory.h"

using namespace AliceO2::Common;
using namespace AliceO2::InfoLogger;
using namespace o2::framework;
using namespace o2::configuration;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace o2::monitoring;

namespace o2::quality_control::core
{

LateTaskRunner::LateTaskRunner(const LateTaskRunnerConfig& config)
  : mTaskConfig(config)
{
  o2::ccdb::BasicCCDBManager::instance().setFatalWhenNull(false);
}

void LateTaskRunner::init(InitContext& iCtx)
{
  try {
    // setup publisher
    mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig.taskName, mTaskConfig.className, mTaskConfig.detectorName, 0);

    // setup user's task
    mTask.reset(LateTaskFactory::create(mTaskConfig, mObjectsManager));

    // init user's task
    mTask->setObjectsManager(mObjectsManager);
    mTask->initialize(iCtx);

    mValidity = gInvalidValidityInterval;

    // todo move to start
    mTask->startOfActivity({});
  } catch (boost::exception& e) {
    ILOG(Info, Devel) << "exception during init " << diagnostic_information(e) << ENDM;
    throw;
  }
}

void LateTaskRunner::run(ProcessingContext& pCtx)
{
  mValidity.update(getCurrentTimestamp());

  // run the task
  mTask->process(pCtx);

  // publish objects
  mObjectsManager->setValidity(mValidity);
  std::unique_ptr<MonitorObjectCollection> array(mObjectsManager->getNonOwningArray());
  auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(mTaskConfig.moSpec);
  pCtx.outputs().snapshot(Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec }, *array);
  mObjectsManager->stopPublishing(PublicationPolicy::Once);
}

/// \brief ID string for all LateTaskRunner devices
std::string LateTaskRunner::createIdString()
{
  return { "qc-late-task" };
}
/// \brief Unified DataOrigin for Quality Control tasks
header::DataOrigin LateTaskRunner::createDataOrigin(const std::string& detectorCode)
{
  std::string originStr = "L";
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

/// \brief Unified DataDescription naming scheme for all tasks
header::DataDescription LateTaskRunner::createDataDescription(const std::string& lateTaskName)
{
  if (lateTaskName.empty()) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Empty lateTaskName for task's data description"));
  }

  return quality_control::core::createDataDescription(lateTaskName, LateTaskRunner::taskDescriptionHashLength);
}


} // namespace o2::quality_control::core
