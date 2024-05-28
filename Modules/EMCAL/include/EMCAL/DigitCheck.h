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

#ifndef QC_MODULE_EMCAL_EMCALDIGITCHECK_H
#define QC_MODULE_EMCAL_EMCALDIGITCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::emcal
{

/// \class  DigitsCheck
/// \brief  Checker for histograms from the digits QC task
/// \ingroup EMCALQCCheckers
/// \author Cristina Terrevoli
/// \since September 30th, 2019
class DigitCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  DigitCheck() = default;
  /// Destructor
  ~DigitCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(DigitCheck, 2);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALDIGITCHECK_H
