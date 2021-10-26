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
/// \file   PostProcessDiagnosticPerCrate.h
/// \brief  Post processing to rearrange TOF information at the level of the crate (maybe we should do the opposite..)
/// \author Nicolo' Jacazio and Francesca Ercolessi
/// \since  11/09/2020
///

#ifndef QUALITYCONTROL_POSTPROCESSDIAGNOSTICPERCRATE_H
#define QUALITYCONTROL_POSTPROCESSDIAGNOSTICPERCRATE_H

// QC includes
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"

#include <array>
#include <memory>
#include <string>

class TH1F;
class TH2F;

namespace o2::quality_control_modules::tof
{

/// \brief  Post processing to rearrange TOF information at the level of the crate (maybe we should do the opposite..)
/// \author Nicolo' Jacazio and Francesca Ercolessi
class PostProcessDiagnosticPerCrate final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  PostProcessDiagnosticPerCrate() = default;
  /// \brief Destructor
  ~PostProcessDiagnosticPerCrate() override;

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
  static const int mNWords;
  static const int mNSlots;

  std::array<std::shared_ptr<TH2F>, 72> mCrates;
  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  static const std::string mCCDBPath;
};

} // namespace o2::quality_control_modules::tof

#endif //QUALITYCONTROL_POSTPROCESSDIAGNOSTICPERCRATE_H
