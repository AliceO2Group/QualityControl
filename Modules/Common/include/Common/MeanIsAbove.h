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
/// \file   MeanIsAbove.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_COMMON_MEANISABOVE_H
#define QC_MODULE_COMMON_MEANISABOVE_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

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
  MeanIsAbove();
  /// Destructor
  ~MeanIsAbove() override = default;

  void configure(std::string name) override;
  Quality check(const MonitorObject* mo) override;
  void beautify(MonitorObject* mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  float mThreshold;

  ClassDefOverride(MeanIsAbove, 1)
};

} // namespace o2::quality_control_modules::common

#endif /* QC_MODULE_COMMON_MEANISABOVE_H */
