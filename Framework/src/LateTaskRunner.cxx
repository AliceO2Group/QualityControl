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

#include <numeric>
#include <utility>

#include <TSystem.h>

#include <Common/Exceptions.h>
#include <Monitoring/Monitoring.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/InitContext.h>
#include <Framework/ConfigParamRegistry.h>

#include "QualityControl/ObjectsManager.h"
#include "QualityControl/LateTaskFactory.h"
#include "QualityControl/QCInputs.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"

using namespace AliceO2::Common;
using namespace o2::framework;
using namespace o2::quality_control::core;

namespace o2::quality_control::core
{
LateTaskRunner::LateTaskRunner(const ServicesConfig& servicesConfig, const LateTaskConfig& config)
  : Actor<LateTaskRunner>(servicesConfig), mTaskConfig(config)
{
}

void LateTaskRunner::onInit(InitContext& iCtx)
{
  // setup publisher
  mObjectsManager = std::make_shared<ObjectsManager>(mTaskConfig.name, mTaskConfig.className, mTaskConfig.detectorName, 0);

  // setup user's task
  mTask.reset(LateTaskFactory::create(mTaskConfig, mObjectsManager));

  // init user's task
  mTask->setObjectsManager(mObjectsManager);
  mTask->initialize(iCtx);
}

void LateTaskRunner::onStart(framework::ServiceRegistryRef services, const Activity& activity)
{
  mTask->startOfActivity(activity);
  mObjectsManager->setActivity(activity);
  mObjectActivity = std::nullopt;
}

void LateTaskRunner::onProcess(ProcessingContext& pCtx)
{
  const QCInputs& taskInputs = extractInputs(pCtx);
  if (taskInputs.size() == 0) {
    ILOG(Warning, Support) << "Could not prepare inputs for task, will not call process() and publish objects. "
                              "Known causes: no requested objects found in input; corrupted input"
                           << ENDM;
    return;
  }

  // run the task
  mTask->process(taskInputs);

  // compute and set correct activity and validity for output objects
  const Activity activity = extractStrictestMatchingActivity(taskInputs);
  mObjectActivity = activity;
  mObjectsManager->setActivity(activity);

  // publish objects
  std::unique_ptr<MonitorObjectCollection> array(mObjectsManager->getNonOwningArray());
  pCtx.outputs().snapshot(mTaskConfig.name, *array);
  mObjectsManager->stopPublishing(PublicationPolicy::Once);
}

void LateTaskRunner::onStop(framework::ServiceRegistryRef services, const Activity& activity)
{
  mTask->endOfActivity(activity);
  if (mObjectActivity.has_value()) {
    mObjectsManager->setActivity(mObjectActivity.value());
  }
  mObjectsManager->stopPublishing(PublicationPolicy::ThroughStop);
}

QCInputs LateTaskRunner::extractInputs(ProcessingContext& pCtx)
{
  QCInputs taskInputs;

  for (const auto& ref : InputRecordWalker(pCtx.inputs())) {
    // InputRecordWalker because the output of CheckRunner can be multi-part
    const auto* inputSpec = ref.spec;
    if (inputSpec == nullptr) {
      continue;
    }

    auto matchingDataSource = std::ranges::find_if(mTaskConfig.dataSources, [&](const auto& dataSource) {
      // all data sources we support are expected to have a single InputSpec
      return dataSource.inputs.size() == 1 && dataSource.inputs[0].binding == inputSpec->binding;
    });

    if (matchingDataSource == mTaskConfig.dataSources.end()) {
      continue;
    }

    if (matchingDataSource->isOneOf(DataSourceType::LateTask, DataSourceType::Task, DataSourceType::TaskMovingWindow)) {
      auto moc = DataRefUtils::as<MonitorObjectCollection>(ref);
      // makes MOs own their TObjects and MOC own MO (the latter we cancel in the next step)
      moc->postDeserialization();
      moc->SetOwner(false);

      for (const auto& obj : *moc) {
        auto mo = dynamic_cast<MonitorObject*>(obj);
        if (mo != nullptr) {
          if (matchingDataSource->subInputs.empty() || std::ranges::find(matchingDataSource->subInputs, mo->getName()) != matchingDataSource->subInputs.end()) {
            taskInputs.insert(mo->getName(), std::shared_ptr<MonitorObject>(mo));
          } else {
            delete mo;
          }
        } else {
          delete obj;
        }
      }
    } else if (matchingDataSource->isOneOf(DataSourceType::Check, DataSourceType::Aggregator)) {
      auto qo = DataRefUtils::as<QualityObject>(ref);
      auto key = qo->getName();
      taskInputs.insert(key, std::shared_ptr<QualityObject>(std::move(qo)));
    } else {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("unsupported data source type: " + std::to_string(static_cast<long>(matchingDataSource->type))));
    }
  }
  return taskInputs;
}

Activity LateTaskRunner::extractStrictestMatchingActivity(const QCInputs& inputs)
{
  // fixme: c++20 can't join ranges, consider refactoring with std::views::concat or similar, when available
  //  Also, having a common parent class for MonitorObject and QualityObject would help.
  std::vector<std::reference_wrapper<const Activity>> activities;

  for (const auto& mo : inputs.iterateByType<MonitorObject>()) {
    activities.emplace_back(mo.getActivity());
  }

  for (const auto& qo : inputs.iterateByType<QualityObject>()) {
    activities.emplace_back(qo.getActivity());
  }

  if (mTaskConfig.outputActivityStrategy == OutputActivityStrategy::Integrated && mObjectActivity.has_value()) {
    activities.emplace_back(mObjectActivity.value());
  }

  return activity_helpers::strictestMatchingActivity(
    activities | //
    std::views::transform([](const auto& ref) -> const Activity& {
      return ref.get();
    }));
}
} // namespace o2::quality_control::core
