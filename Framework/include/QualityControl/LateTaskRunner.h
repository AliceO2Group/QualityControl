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

#ifndef QC_CORE_LATETASKRUNNER_H
#define QC_CORE_LATETASKRUNNER_H

///
/// \file   LateTaskRunner.h
/// \author Piotr Konopka
///

#include "QualityControl/LateTaskConfig.h"
#include "QualityControl/LateTaskRunnerTraits.h"

#include "QualityControl/ActorTraits.h"
#include "QualityControl/Actor.h"
#include <string_view>
#include <memory>

namespace o2::quality_control::core {

class LateTaskInterface;
class ObjectsManager;

class LateTaskRunner : public Actor<LateTaskRunner>
{
 public:
  LateTaskRunner(const ServicesConfig& servicesConfig, const LateTaskConfig& config);
  ~LateTaskRunner() = default;

  void onStart(framework::ServiceRegistryRef services);
  void onInit(framework::InitContext& iCtx);
  void onProcess(framework::ProcessingContext& pCtx);

  std::string_view getDetectorName() const { return mTaskConfig.detectorName; }
  std::string_view getUserCodeName() const { return mTaskConfig.name; }
  bool isCritical() const { return mTaskConfig.critical; }

 private:

  void startOfActivity();
  void endOfActivity();
  int publish(framework::DataAllocator& outputs);

 private:
  LateTaskConfig mTaskConfig;
  std::shared_ptr<LateTaskInterface> mTask;
  std::shared_ptr<ObjectsManager> mObjectsManager;
  ValidityInterval mValidity;
};



}
#endif //QC_CORE_LATETASKRUNNER_H
