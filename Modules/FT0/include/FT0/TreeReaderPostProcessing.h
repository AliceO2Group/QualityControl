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
/// \file   TreeReaderPostProcessing.h
/// \author My Name
///

#ifndef QUALITYCONTROL_TREEREADERPOSTPROCESSING_H
#define QUALITYCONTROL_TREEREADERPOSTPROCESSING_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"

class TH1F;

namespace o2::quality_control_modules::ft0
{

/// \brief Example Quality Control Postprocessing Task
/// \author My Name
class TreeReaderPostProcessing final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  TreeReaderPostProcessing() = default;
  /// \brief Destructor
  ~TreeReaderPostProcessing() override;

  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;

 private:
 o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  std::unique_ptr<TH1F> mChargeHistogram;
};

} // namespace o2::quality_control_modules::skeleton

#endif //QUALITYCONTROL_TreeReaderPostProcessing_H