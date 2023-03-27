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
/// \file   OutOfBunchCollCheck.h
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#ifndef QC_MODULE_FDD_FDDOUTOFBUNCHCOLLCHECK_H
#define QC_MODULE_FDD_FDDOUTOFBUNCHCOLLCHECK_H

#include "QualityControl/CheckInterface.h"
#include "CommonConstants/LHCConstants.h"

namespace o2::quality_control_modules::fdd
{

/// \brief  Checks what fraction of collisions is out of bunch
/// \author Sebastian Bysiak sbysiak@cern.ch
class OutOfBunchCollCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  OutOfBunchCollCheck() = default;
  /// Destructor
  ~OutOfBunchCollCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  constexpr static std::size_t sBCperOrbit = o2::constants::lhc::LHCMaxBunches;

  ClassDefOverride(OutOfBunchCollCheck, 2);

 private:
  float mFractionOutOfBunchColl;
  int mNumNonEmptyBins;
  float mThreshWarning;
  float mThreshError;
  std::string mTrgName;
  int mBinPos;
  bool mEnableMessage{ true };
};

} // namespace o2::quality_control_modules::fdd

#endif // QC_MODULE_FDD_FDDOUTOFBUNCHCOLLCHECK_H
