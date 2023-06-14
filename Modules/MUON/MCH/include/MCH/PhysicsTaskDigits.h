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
/// \file   PhysicsTaskDigits.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_PHYSICSTASKDIGITS_H
#define QC_MODULE_MUONCHAMBERS_PHYSICSTASKDIGITS_H

#include "QualityControl/TaskInterface.h"

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

/// \brief Dummy task for the compatibility with the current full system test
/// \author Andrea Ferrero
class PhysicsTaskDigits /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  PhysicsTaskDigits() = default;
  /// Destructor
  ~PhysicsTaskDigits() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override {}
  void startOfActivity(const Activity& activity) override {}
  void startOfCycle() override {}
  void monitorData(o2::framework::ProcessingContext& ctx) override {}
  void endOfCycle() override {}
  void endOfActivity(const Activity& activity) override {}
  void reset() override {}
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_PHYSICSTASKDIGITS_H
