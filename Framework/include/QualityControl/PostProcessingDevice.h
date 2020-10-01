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
/// \file   PostProcessingDevice.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGDEVICE_H
#define QUALITYCONTROL_POSTPROCESSINGDEVICE_H


#include <Framework/Task.h>
#include <Framework/DataProcessorSpec.h>
#include <Headers/DataHeader.h>

#include <string>
#include <memory>

namespace o2::quality_control::postprocessing
{

class PostProcessingRunner;

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
  /// \param id - subSpecification for taskRunner's OutputSpec, useful to avoid outputs collisions one more complex topologies
  PostProcessingDevice(const std::string& taskName, const std::string& configurationSource);
  ~PostProcessingDevice() override = default;

  /// \brief TaskRunner's init callback
  void init(framework::InitContext&) override;
  /// \brief TaskRunner's process callback
  void run(framework::ProcessingContext&) override;

  const std::string& getDeviceName();
  framework::Inputs getInputsSpecs();
  framework::Outputs getOutputSpecs();
  framework::Options getOptions();

  /// \brief ID string for all TaskRunner devices
  static std::string createPostProcessingIdString();
  /// \brief Unified DataOrigin for Quality Control tasks
  static header::DataOrigin createPostProcessingDataOrigin();
  /// \brief Unified DataDescription naming scheme for all tasks
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
  std::string mConfigSource;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_POSTPROCESSINGDEVICE_H
