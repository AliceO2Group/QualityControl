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
/// \author Cristina Terrevoli
///

#ifndef QC_MODULE_EMCAL_EMCALRAWCHECK_H
#define QC_MODULE_EMCAL_EMCALRAWCHECK_H

#include "QualityControl/CheckInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

/// \brief EMCAL raw data check
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
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(RawCheck, 1);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWCHECK_H
