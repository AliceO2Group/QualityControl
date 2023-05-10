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
/// \file   PhysicsCheck.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_CPV_CPVPHYSICSCHECK_H
#define QC_MODULE_CPV_CPVPHYSICSCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::cpv
{

/// \brief  CPV PhysicsCheck
/// \author Sergey Evdokimov
///
class PhysicsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PhysicsCheck() = default;
  /// Destructor
  ~PhysicsCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override { return "TH1"; }

 private:
  int getRunNumberFromMO(std::shared_ptr<MonitorObject> mo);

  // configurable parameters and their default values
  // see config example in Modules/CPV/etc/Physics-task-no-sampling.json
  float mAmplitudeRangeL[3] = { 20., 20., 20. };
  float mAmplitudeRangeR[3] = { 1000., 1000., 1000. };
  float mMinEventsToFit[3] = { 100, 100, 100 };
  float mMinAmplification[3] = { 5., 5., 5. };
  float mMaxAmplification[3] = { 200., 200., 200. };
  float mMinClusterSize[3] = { 2., 2., 2. };
  float mMaxClusterSize[3] = { 5., 5., 5. };
  int mMinEventsToCheckDigitMap[3] = { 10000, 10000, 10000 };
  float mStripPopulationDeviationAllowed[3] = { 10., 10., 10. };
  int mNBadStripsPerQuarterAllowed[3] = { 10, 10, 10 };
  int mNCold3GassiplexAllowed[3] = { 10, 10, 10 };
  int mNHot3GassiplexAllowed[3] = { 10, 10, 10 };
  float mHot3GassiplexCriterium[3] = { 10., 10., 10. };
  float mCold3GassiplexCriterium[3] = { 0.1, 0.1, 0.1 };

  bool mIsConfigured = false; // had configure() been called already?

  ClassDefOverride(PhysicsCheck, 1);
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPHYSICSCHECK_H
