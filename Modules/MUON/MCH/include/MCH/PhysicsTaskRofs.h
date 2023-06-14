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
/// \file   PhysicsTaskRofs.h
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#ifndef QC_MODULE_MCH_PHYSICSTASKROFS_H
#define QC_MODULE_MCH_PHYSICSTASKROFS_H

#include "QualityControl/TaskInterface.h"

using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

/// \brief Quality Control Task for the analysis of raw data decoding errors
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class PhysicsTaskRofs /*final*/ : public TaskInterface
{
 public:
  /// \brief Constructor
  PhysicsTaskRofs() = default;
  /// Destructor
  ~PhysicsTaskRofs() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override {}
  void startOfActivity(const Activity& activity) override {}
  void startOfCycle() override {}
  void monitorData(o2::framework::ProcessingContext& ctx) override {}
  void endOfCycle() override {}
  void endOfActivity(const Activity& activity) override {}
  void reset() override {}
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DECODINGERRORSTASK_H
