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
/// \file   MeanIsAbove.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_COMMON_MEANISABOVE_H
#define QC_MODULE_COMMON_MEANISABOVE_H

#include "QualityControl/CheckInterface.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

/// \brief  Check whether the mean of the plot is above a certain limit.
///
/// \author Barthelemy von Haller
class MeanIsAbove : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  MeanIsAbove() = default;
  /// Destructor
  ~MeanIsAbove() override = default;

  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  float mThreshold = 0.0f;

  ClassDefOverride(MeanIsAbove, 2)
};

} // namespace o2::quality_control_modules::common

#endif /* QC_MODULE_COMMON_MEANISABOVE_H */
