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
/// \author Chiara Zampolli
///

#ifndef QC_MODULE_GLO_ITSTPCMATCHINGTASK_H
#define QC_MODULE_GLO_ITSTPCMATCHINGTASK_H

#include "QualityControl/TaskInterface.h"
#include "GLOQC/MatchITSTPCQC.h"
#include "Common/TH1Ratio.h"

#include <memory>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::glo
{

/// \brief ITS-TPC Matching QC task
/// \author Chiara Zampolli
class ITSTPCMatchingTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ITSTPCMatchingTask() = default;
  /// Destructor
  ~ITSTPCMatchingTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  o2::gloqc::MatchITSTPCQC mMatchITSTPCQC;

  bool mIsSync{ false };
  bool mDoMTCRatios{ false };
  std::unique_ptr<common::TH1FRatio> mEffPt;
  std::unique_ptr<common::TH1FRatio> mEffEta;
  std::unique_ptr<common::TH1FRatio> mEffPhi;
};

} // namespace o2::quality_control_modules::glo

#endif // QC_MODULE_GLO_ITSTPCMATCHINGTASK_H
