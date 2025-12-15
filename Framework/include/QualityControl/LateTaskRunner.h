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

#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/CompletionPolicy.h>

#include "QualityControl/LateTaskRunnerConfig.h"

namespace o2::quality_control::core {

class LateTaskInterface;
class ObjectsManager;

class LateTaskRunner : public framework::Task
{
  /// \brief Number of bytes in data description used for hashing of Task names. See HashDataDescription.h for details
  static constexpr size_t taskDescriptionHashLength = 4;
  static_assert(taskDescriptionHashLength <= o2::header::DataDescription::size);

 public:
  explicit LateTaskRunner(const LateTaskRunnerConfig& config);
  ~LateTaskRunner() override = default;

  /// \brief LateTaskRunner's init callback
  void init(framework::InitContext& iCtx) override;
  /// \brief LateTaskRunner's process callback
  void run(framework::ProcessingContext& pCtx) override;
  /// \brief LateTaskRunner's finaliseCCDB callback
  // void finaliseCCDB(framework::ConcreteDataMatcher& matcher, void* obj) override;

  std::string getDeviceName() const { return mTaskConfig.deviceName; };
  const framework::Inputs& getInputsSpecs() const { return mTaskConfig.inputSpecs; };
  const framework::OutputSpec& getOutputSpec() const { return mTaskConfig.moSpec; };
  const framework::Options& getOptions() const { return mTaskConfig.options; };

  /// \brief Data Processor Label to identify all Late Task Runners
  static framework::DataProcessorLabel getLabel() { return { "qc-late-task" }; }
  /// \brief ID string for all LateTaskRunner devices
  static std::string createIdString();
  /// \brief Unified DataOrigin for Quality Control tasks
  static header::DataOrigin createDataOrigin(const std::string& detectorCode);
  /// \brief Unified DataDescription naming scheme for all tasks
  static header::DataDescription createDataDescription(const std::string& taskName);

  /// \brief Callback for CallbackService::Id::EndOfStream
  // void endOfStream(framework::EndOfStreamContext& eosContext) override;

 private:
  /// \brief Callback for CallbackService::Id::Start (DPL) a.k.a. RUN transition (FairMQ)
  // void start(framework::ServiceRegistryRef services);
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  // void stop(framework::ServiceRegistryRef services);
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  // void reset();

  void startOfActivity();
  void endOfActivity();
  int publish(framework::DataAllocator& outputs);
  // void publishCycleStats();
  // void updateMonitoringStats(framework::ProcessingContext& pCtx);
  // void registerToBookkeeping();

 private:
  LateTaskRunnerConfig mTaskConfig;
  // std::shared_ptr<monitoring::Monitoring> mCollector;
  std::shared_ptr<LateTaskInterface> mTask;
  std::shared_ptr<ObjectsManager> mObjectsManager;
  // std::shared_ptr<Timekeeper> mTimekeeper;
  Activity mActivity;
  ValidityInterval mValidity;

  // stats
  // int mNumberMessagesReceivedInCycle = 0;
  // int mNumberObjectsPublishedInCycle = 0;
  // int mTotalNumberObjectsPublished = 0; // over a run
  // double mLastPublicationDuration = 0;
  // uint64_t mDataReceivedInCycle = 0;
};


}
#endif //QC_CORE_LATETASKRUNNER_H
