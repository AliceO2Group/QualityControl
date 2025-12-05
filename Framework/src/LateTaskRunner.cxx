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

LateTaskRunner::LateTaskRunner(const ServicesConfig& servicesConfig, const LateTaskConfig& config)
: Actor<LateTaskRunner>(servicesConfig), mTaskConfig(config)
{
  // fixme: this should be moved to Actor
  o2::ccdb::BasicCCDBManager::instance().setFatalWhenNull(false);
}

void LateTaskRunner::onInit(InitContext& iCtx)
{
  // fixme: move exception handling to Actor
  try {
    // setup publisher
    mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig.name, mTaskConfig.className, mTaskConfig.detectorName, 0);

    // setup user's task
    mTask.reset(LateTaskFactory::create(mTaskConfig, mObjectsManager));

    // init user's task
    mTask->setObjectsManager(mObjectsManager);
    mTask->initialize(iCtx);

    mValidity = gInvalidValidityInterval;

    // todo move to start
    mTask->startOfActivity(getActivity());
    mObjectsManager->setActivity(getActivity());
  } catch (boost::exception& e) {
    ILOG(Info, Devel) << "exception during init " << diagnostic_information(e) << ENDM;
    throw;
  }
}

void LateTaskRunner::onProcess(ProcessingContext& pCtx)
{
  // todo: derive from received objects
  mValidity.update(getCurrentTimestamp());

  QCInputs taskInputs;
  for (const auto& ref : InputRecordWalker(pCtx.inputs())) {
    // InputRecordWalker because the output of CheckRunner can be multi-part
    const auto* inputSpec = ref.spec;
    if (inputSpec == nullptr) {
      continue;
    }
    const auto dataOrigin = DataSpecUtils::asConcreteOrigin(*inputSpec);

    // fixme: come up with an elegant way for this. LateTask should be probably aware of the requested user inputs aside from just InputSpecs
    //  also, we should filter only expect objects from the received collections
    if (dataOrigin.str[0] == 'Q' || dataOrigin.str[0] == 'W' || dataOrigin.str[0] == 'L') { // main MOs and moving windows from QC tasks, and outputs of late tasks
      auto moc = DataRefUtils::as<MonitorObjectCollection>(ref);
      moc->postDeserialization();

      for (const auto& obj : *moc) {
        auto mo = dynamic_cast<MonitorObject*>(obj);
        if (mo != nullptr) {
          taskInputs.insert(mo->getName(), std::shared_ptr<MonitorObject>(mo));
        }
      }

      moc->SetOwner(false);
    } else if (dataOrigin.str[0] == 'C' || dataOrigin.str[0] == 'A') { // QOs from Checks and Aggregators
      // QO
      auto qo = DataRefUtils::as<QualityObject>(ref);
      auto key = qo->getName();
      taskInputs.insert(key, std::shared_ptr<QualityObject>(std::move(qo)));

    } else {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("LateTaskRunner currently supports only MonitorObject and Quality inputs"));
    }
  }

  // run the task
  mTask->process(taskInputs);

  // publish objects
  mObjectsManager->setValidity(mValidity);
  std::unique_ptr<MonitorObjectCollection> array(mObjectsManager->getNonOwningArray());
  pCtx.outputs().snapshot(mTaskConfig.name, *array);
  mObjectsManager->stopPublishing(PublicationPolicy::Once);
}


} // namespace o2::quality_control::core
