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
/// \file   PulsePositionCheck.h
/// \author Deependra (deependra.sharma@cern.ch)
///

#ifndef QC_MODULE_TRD_TRDPULSEPOSITIONCHECK_H
#define QC_MODULE_TRD_TRDPULSEPOSITIONCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::trd
{

/// \brief  Example QC Check
/// \author My Name
class PulsePositionCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PulsePositionCheck() = default;
  /// Destructor
  ~PulsePositionCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  long int mTimeStamp;
  std::pair<float, float> mPulseHeightPeakRegion;
  double chi2byNDF_threshold;
  double FitParam0;
  double FitParam1;
  double FitParam2;
  double FitParam3;
  double FunctionRange[2];
  double FitRange[2];

  ClassDefOverride(PulsePositionCheck, 2);
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDPULSEPOSITIONCHECK_H