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
/// \file   CalibMQcCheck.h
/// \author Valerie Ramillien
///

#ifndef QC_MODULE_MID_MIDCALIBMQCCHECK_H
#define QC_MODULE_MID_MIDCALIBMQCCHECK_H

#include "QualityControl/CheckInterface.h"

#include "MID/HistoHelper.h"

namespace o2::quality_control_modules::mid
{

class CalibMQcCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CalibMQcCheck() = default;
  /// Destructor
  ~CalibMQcCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  ///////////////////////////
  HistoHelper mHistoHelper; ///! Histogram helper
  ClassDefOverride(CalibMQcCheck, 3);
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDCALIBMQCCHECK_H
