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
/// \file   ITSTPCMatchingTask.h
/// \author My Name
///

#ifndef QC_MODULE_GLO_ITSTPCMATCHINGTASK_H
#define QC_MODULE_GLO_ITSTPCMATCHINGTASK_H

#include "QualityControl/TaskInterface.h"
#include "GlobalTracking/MatchITSTPCQC.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::glo
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class ITSTPCMatchingTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ITSTPCMatchingTask() = default;
  /// Destructor
  ~ITSTPCMatchingTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  o2::globaltracking::MatchITSTPCQC mMatchITSTPCQC;
};

} // namespace o2::quality_control_modules::glo

#endif // QC_MODULE_GLO_ITSTPCMATCHINGTASK_H
