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
/// \file   ITSTPCmatchingCheck.h
/// \author felix.schlepper@cern.ch
///

#ifndef QC_MODULE_GLO_GLOITSTPCMATCHINGCHECK_H
#define QC_MODULE_GLO_GLOITSTPCMATCHINGCHECK_H

#include "QualityControl/CheckInterface.h"

#include <vector>
#include <tuple>

namespace o2::quality_control_modules::glo
{

/// \brief  Check for ITS-TPC sync. matching efficiency
/// \author felix.schlepper@cern.ch
class ITSTPCmatchingCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ITSTPCmatchingCheck() = default;
  /// Destructor
  ~ITSTPCmatchingCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  void reset() override;
  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;

 private:
  std::vector<std::pair<int, int>> findRanges(const std::vector<int>& nums);
  std::shared_ptr<Activity> mActivity;

  float mMinPt{ 1. };
  float mMaxPt{ 1.999 };
  float mThreshold{ 0.5 };

  ClassDefOverride(ITSTPCmatchingCheck, 1);
};

} // namespace o2::quality_control_modules::glo

#endif // QC_MODULE_GLO_GLOITSTPCMATCHINGCHECK_H
