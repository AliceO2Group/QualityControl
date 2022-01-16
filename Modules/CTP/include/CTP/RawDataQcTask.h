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
/// \file   RawDataQcTask.h
/// \author Marek Bombara
///

#ifndef QC_MODULE_CTP_CTPRAWDATAQCTASK_H
#define QC_MODULE_CTP_CTPRAWDATAQCTASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ctp
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class RawDataQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  RawDataQcTask() = default;
  /// Destructor
  ~RawDataQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TH1F* mHistoBC = nullptr;
  TH1F* mHistoInputs = nullptr;
  TH1F* mHistoClasses = nullptr;
};

} // namespace o2::quality_control_modules::ctp

#endif // QC_MODULE_CTP_CTPRAWDATAQCTASK_H
