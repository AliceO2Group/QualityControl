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
/// \file   PostProcessingDevice.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGDEVICE_H
#define QUALITYCONTROL_POSTPROCESSINGDEVICE_H

#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Headers/DataHeader.h>
#include "QualityControl/PostProcessingRunnerConfig.h"

#include <string>
#include <memory>

namespace o2::quality_control::postprocessing
{

class PostProcessingRunner;
struct PostProcessingRunnerConfig;

/// \brief A class driving the execution of a QC PostProcessing task inside DPL.
///
/// \author Piotr Konopka
class PostProcessingDevice : public framework::Task
{
 public:
  /// \brief Constructor
  ///
  /// \param taskName - name of the task, which exists in tasks list in the configuration file
  /// \param configurationSource - absolute path to configuration file, preceded with backend (f.e. "json://")
  PostProcessingDevice(const PostProcessingRunnerConfig& runnerConfig);
  ~PostProcessingDevice() override = default;

  /// \brief PostProcessingDevice's init callback
  void init(framework::InitContext&) override;
  /// \brief PostProcessingDevice's process callback
  void run(framework::ProcessingContext&) override;

  const std::string& getDeviceName();
  framework::Inputs getInputsSpecs();
  framework::Outputs getOutputSpecs();
  framework::Options getOptions();

  /// \brief ID string for all PostProcessingDevices
  static std::string createPostProcessingIdString();
  /// \brief Unified DataOrigin for Post-processing tasks
  static header::DataOrigin createPostProcessingDataOrigin();
  /// \brief Unified DataDescription naming scheme for all Post-processing tasks
  static header::DataDescription createPostProcessingDataDescription(const std::string& taskName);

 private:
  /// \brief Callback for CallbackService::Id::Start (DPL) a.k.a. RUN transition (FairMQ)
  void start();
  /// \brief Callback for CallbackService::Id::Stop (DPL) a.k.a. STOP transition (FairMQ)
  void stop();
  /// \brief Callback for CallbackService::Id::Reset (DPL) a.k.a. RESET DEVICE transition (FairMQ)
  void reset();

 private:
  std::shared_ptr<PostProcessingRunner> mRunner;
  std::string mDeviceName;
  PostProcessingRunnerConfig mRunnerConfig;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGDEVICE_H
