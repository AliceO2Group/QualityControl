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
/// \file   RawCheck.h
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#ifndef QC_MODULE_PHOS_PHOSRAWCHECK_H
#define QC_MODULE_PHOS_PHOSRAWCHECK_H

#include "QualityControl/CheckInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::phos
{

/// \brief PHOS raw data check
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Cristina Terrevoli
class RawCheck final : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  RawCheck() = default;
  /// Destructor
  ~RawCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override {}
  std::string getAcceptedType() override;

 protected:
  bool checkErrHistograms(MonitorObject* mo);
  bool checkPhysicsHistograms(MonitorObject* mo);
  bool checkPedestalHistograms(MonitorObject* mo);

 private:
  int mMinHGPedestalValue = 18;
  int mMaxHGPedestalValue = 60;
  int mMinLGPedestalValue = 18;
  int mMaxLGPedestalValue = 60;
  float mMinHGPedestalRMS = 0.2;
  float mMaxHGPedestalRMS = 2.5;
  float mMinLGPedestalRMS = 0.2;
  float mMaxLGPedestalRMS = 2.5;

  int mToleratedBadChannelsM[5] = { 0, 10, 20, 20, 20 }; // Nuber of channels beyond bad map
  int mBadMap[5] = { 0 };
  float mErrorOccuranceThreshold[5] = { 0., 0., 0., 0., 0. };
  const char* mErrorLabel[5] = { "wrong ALTRO", "mapping", "ch. header",
                                 "ch. payload", "wrond hw addr" };
  float mBranchOccupancyDeviationAllowed[5] = { 1.5, 1.5, 1.5, 1.5, 1.5 };
  int mToleratedDeviatedBranches[5] = { 0, 0, 0, 0, 0 };
  Quality mCheckResult = Quality::Null;
  ClassDefOverride(RawCheck, 2);
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_PHOSRAWCHECK_H
