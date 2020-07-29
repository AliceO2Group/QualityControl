// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   CheckRawToT.h
/// \author Nicolo' Jacazio
/// \brief  Checker for TOF Raw data on ToT
///

#ifndef QC_MODULE_TOF_TOFCHECKRAWSTOT_H
#define QC_MODULE_TOF_TOFCHECKRAWSTOT_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::tof
{

class CheckRawToT : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckRawToT() = default;
  /// Destructor
  ~CheckRawToT() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  /// Minimum ToT allowed for the mean in ns
  float mMinRawToT;
  /// Maximum ToT allowed for the mean in ns
  float mMaxRawToT;

  ClassDefOverride(CheckRawToT, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFCHECKRAWSTOT_H
