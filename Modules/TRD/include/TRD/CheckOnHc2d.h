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
/// \file   CheckOnHc2d.h
/// \author My Name
///

#ifndef QC_MODULE_TRD_TRDCHECKONHC2D_H
#define QC_MODULE_TRD_TRDCHECKONHC2D_H

// O2
#include "CCDB/BasicCCDBManager.h"
#include "DataFormatsTRD/NoiseCalibration.h"
#include <DataFormatsTRD/Constants.h>

// QC
#include "QualityControl/CheckInterface.h"

using namespace o2::trd::constants;

namespace o2::quality_control_modules::trd
{

/// \brief  Example QC Check
/// \author My Name
class CheckOnHc2d : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckOnHc2d() = default;
  /// Destructor
  ~CheckOnHc2d() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  void reset() override;
  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;

 private:
  int mTimestamp;
  o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
  const std::array<int, o2::trd::constants::MAXCHAMBER>* mChamberStatus = nullptr;
  std::shared_ptr<Activity> mActivity;
  std::string mCcdbPath;

  ClassDefOverride(CheckOnHc2d, 3);
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDCHECKONHC2D_H
