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
/// \file   SkeletonPostProcessing.h
/// \author My Name
///

#ifndef QUALITYCONTROL_SKELETONPOSTPROCESSING_H
#define QUALITYCONTROL_SKELETONPOSTPROCESSING_H

#include "QualityControl/PostProcessingInterface.h"

class TH1F;

namespace o2::quality_control_modules::skeleton
{

/// \brief Example Quality Control Postprocessing Task
/// \author My Name
class SkeletonPostProcessing final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  SkeletonPostProcessing() = default;
  /// \brief Destructor
  ~SkeletonPostProcessing() override;

  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  TH1F* mHistogram = nullptr;
};

} // namespace o2::quality_control_modules::skeleton

#endif //QUALITYCONTROL_SKELETONPOSTPROCESSING_H