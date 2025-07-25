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
/// \file   GPUErrorQA.h
/// \author Anton Riedel, anton.riedel@cern.ch
///

#ifndef QC_MODULE_TPC_GPUERRORQA_H
#define QC_MODULE_TPC_GPUERRORQA_H

// O2 includes
#include "TPCQC/GPUErrorQA.h"

// QC includes
#include "QualityControl/TaskInterface.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tpc
{

/// \brief Quality Control DPL Task for QC Module TPC GPU errors
/// \author Anton Riedel

class GPUErrorQA final : public TaskInterface
{
 public:
  /// \brief Constructor
  GPUErrorQA() = default;
  /// Destructor
  ~GPUErrorQA() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  o2::tpc::qc::GPUErrorQA mGPUErrorQA; ///< TPC QC class from o2
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_GPUERRORQA_H
