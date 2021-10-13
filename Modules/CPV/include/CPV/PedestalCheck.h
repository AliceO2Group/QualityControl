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
/// \file   PedestalCheck.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_CPV_CPVPEDESTALCHECK_H
#define QC_MODULE_CPV_CPVPEDESTALCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::cpv
{

/// \brief  CPV PedestalCheck
/// \author Sergey Evdokimov
///
class PedestalCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PedestalCheck() = default;
  /// Destructor
  ~PedestalCheck() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  int getRunNumberFromMO(std::shared_ptr<MonitorObject> mo);

  //configurable parameters and their default values
  //see config example in Modules/CPV/etc/pedestal-task-no-sampling.json
  int mMinGoodPedestalValueM[3] = { 1, 1, 1 };
  float mMaxGoodPedestalSigmaM[3] = { 2., 2., 2. };
  float mMinGoodPedestalEfficiencyM[3] = { 0.7, 0.7, 0.7 };
  float mMaxGoodPedestalEfficiencyM[3] = { 1., 1., 1. };
  int mToleratedBadPedestalValueChannelsM[3] = { 10, 10, 10 };      //pedestal value < mMinGoodPedestalValue or > 512
  int mToleratedBadChannelsM[3] = { 20, 20, 20 };                   //double peaks or empty or channels
  int mToleratedBadPedestalSigmaChannelsM[3] = { 20, 20, 20 };      //pedestal value < mMinGoodPedestalValue or > 512
  int mToleratedBadPedestalEfficiencyChannelsM[3] = { 20, 20, 20 }; //efficiency < min or > max

  ClassDefOverride(PedestalCheck, 1);
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPEDESTALCHECK_H
