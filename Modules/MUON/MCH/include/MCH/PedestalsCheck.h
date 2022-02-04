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
/// \file   PedestalsCheck.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MCH_PEDESTALSCHECK_H
#define QC_MODULE_MCH_PEDESTALSCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class PedestalsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PedestalsCheck();
  /// Destructor
  ~PedestalsCheck() override;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  /// Minimum value for SAMPA pedestals
  float minMCHpedestal;
  /// Maximum value for SAMPA pedestals
  float maxMCHpedestal;

 private:
  /// Vector filled with DualSampas Ids that have been tested but sent back no data
  std::vector<int> missing;

  ClassDefOverride(PedestalsCheck, 2);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
