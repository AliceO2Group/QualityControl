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
/// \file   PostProcessingRunner.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGRUNNER_H
#define QUALITYCONTROL_POSTPROCESSINGRUNNER_H

#include <memory>
#include <functional>
#include <Framework/ServiceRegistry.h>
#include <boost/property_tree/ptree_fwd.hpp>
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/PostProcessingRunnerConfig.h"
#include "QualityControl/PostProcessingTaskSpec.h"
#include "QualityControl/Triggers.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/MonitorObjectCollection.h"

namespace o2::framework
{
class DataAllocator;
} // namespace o2::framework

namespace o2::quality_control::core
{
struct CommonSpec;
class Activity;
}

namespace o2::quality_control::postprocessing
{

using MOCPublicationCallback = std::function<void(const o2::quality_control::core::MonitorObjectCollection*, long from, long to)>;

/// \brief A class driving the execution of a post-processing task
///
/// It is responsible for setting up a post-processing task and executing its methods corresponding to its state. The
/// state transitions are determined by triggers defined by user.
///
/// \author Piotr Konopka
class PostProcessingRunner
{
 public:
  PostProcessingRunner(std::string name);
  ~PostProcessingRunner() = default;

  /// \brief Initialization. Creates configuration structures out of the ptree. Throws on errors.
  void init(const boost::property_tree::ptree& config);
  /// \brief Initialization. Throws on errors.
  void init(const PostProcessingRunnerConfig& runnerConfig, const PostProcessingConfig& taskConfig);
  /// \brief One iteration over the event loop. Throws on errors. Returns false when it can gracefully exit.
  bool run();
  /// \brief Start transition. Throws on errors.
  void start();
  /// \brief Stop transition. Throws on errors.
  void stop();
  /// \brief Reset transition. Throws on errors.
  void reset();
  /// \brief Runs the task over selected timestamps, performing the full start, run, stop cycle.
  ///
  /// \param t A vector with timestamps (ms since epoch).
  ///          The first is used for task initialisation, the last for task finalisation, so at least two are required.
  void runOverTimestamps(const std::vector<uint64_t>& t);

  /// \brief Set how objects should be published. If not used, objects will be stored in repository.
  ///
  /// \param callback MonitorObjectCollection publication callback
  void setPublicationCallback(MOCPublicationCallback callback);

  const std::string& getName();

  static PostProcessingRunnerConfig extractConfig(const core::CommonSpec& commonSpec, const PostProcessingTaskSpec& ppTaskSpec);

 private:
  void doInitialize(Trigger trigger);
  void doUpdate(Trigger trigger);
  void doFinalize(Trigger trigger);

  enum class TaskState {
    INVALID,
    Created,
    Running,
    Finished
  };
  TaskState mTaskState = TaskState::INVALID;
  std::vector<TriggerFcn> mInitTriggers;
  std::vector<TriggerFcn> mUpdateTriggers;
  std::vector<TriggerFcn> mStopTriggers;

  std::unique_ptr<PostProcessingInterface> mTask;
  framework::ServiceRegistry mServices;
  std::shared_ptr<o2::quality_control::core::ObjectsManager> mObjectManager;
  // TODO in a longer run, we should store from/to in the MonitorObject itself and use them.
  std::function<void(const o2::quality_control::core::MonitorObjectCollection*, long /*from*/, long /*to*/)> mPublicationCallback = nullptr;

  std::string mName{};
  PostProcessingConfig mTaskConfig;
  PostProcessingRunnerConfig mRunnerConfig;
  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
};

MOCPublicationCallback publishToDPL(o2::framework::DataAllocator&, std::string outputBinding);
MOCPublicationCallback publishToRepository(o2::quality_control::repository::DatabaseInterface&);

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGRUNNER_H
