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
/// \file   ExpertVis.h
/// \author Berkin Ulukutlu & Jens Wiechula
///

#ifndef QC_MODULE_TPC_EXPERTVIS_H
#define QC_MODULE_TPC_EXPERTVIS_H

// O2 includes
#include "TPCQC/ExpertVis.h"

// QC includes
#include "QualityControl/TaskInterface.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tpc
{

/// \brief TPC ExpertVis QC Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Berkin Ulukutlu & Jens Wiechula
class ExpertVis final : public TaskInterface
{
 public:
  /// \brief Constructor
  ExpertVis() = default;
  /// Destructor
  ~ExpertVis() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  o2::tpc::qc::ExpertVis mQCExpertVis{};
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_EXPERTVIS_H
