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
/// \file   RawErrorCheck.h
/// \author Markus Fasel
///

#ifndef QC_MODULE_EMCAL_EMCALRAWERRORCHECK_H
#define QC_MODULE_EMCAL_EMCALRAWERRORCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::emcal
{

/// \class RawErrorCheck
/// \brief Checker for histograms with error code published by the RawErrorTask
/// \author Markus Fasel
///
/// Checking for presence of an error code. Any presence of error code (non-0 entry)
/// will define data as bad.
class RawErrorCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  RawErrorCheck() = default;
  /// Destructor
  ~RawErrorCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(RawErrorCheck, 2);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWERRORCHECK_H
