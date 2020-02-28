// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
#include <Framework/ServiceRegistry.h>
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/Triggers.h"
#include "QualityControl/DatabaseInterface.h"

namespace o2::configuration
{
class ConfigurationInterface;
}

namespace o2::quality_control::postprocessing
{

/// \brief A class driving the execution of a post-processing task
///
/// It is responsible for setting up a post-processing task and executing its methods corresponding to its state. The
/// state transitions are determined by triggers defined by user.
///
/// \author Piotr Konopka
class PostProcessingRunner
{
 public:
  PostProcessingRunner(std::string name, std::string configPath);
  ~PostProcessingRunner();

  /// \brief Initialization. Throws on errors.
  void init();
  /// \brief One iteration over the event loop. Throws on errors. Returns false when it can gracefully exit.
  bool run();
  /// \brief Stop transition. Throws on errors.
  void stop();
  /// \brief Reset transition. Throws on errors.
  void reset();

 private:
  enum class TaskState {
    INVALID,
    Created,
    Running,
    Finished
  };
  TaskState mState = TaskState::INVALID;
  std::vector<TriggerFcn> mInitTriggers;
  std::vector<TriggerFcn> mUpdateTriggers;
  std::vector<TriggerFcn> mStopTriggers;

  std::unique_ptr<PostProcessingInterface> mTask;
  framework::ServiceRegistry mServices;

  std::shared_ptr<o2::quality_control::repository::DatabaseInterface> mDatabase;
  std::shared_ptr<configuration::ConfigurationInterface> mConfigFile;
  PostProcessingConfig mConfig;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGRUNNER_H
