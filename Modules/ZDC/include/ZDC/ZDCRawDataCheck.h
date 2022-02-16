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
/// \file   ZDCRawDataCheck.h
/// \author My Name
///

#ifndef QC_MODULE_ZDC_ZDCZDCRAWDATACHECK_H
#define QC_MODULE_ZDC_ZDCZDCRAWDATACHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::zdc
{

/// \brief  Example QC Check
/// \author My Name
class ZDCRawDataCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ZDCRawDataCheck() = default;
  /// Destructor
  ~ZDCRawDataCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(ZDCRawDataCheck, 2);
};

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_ZDCZDCRAWDATACHECK_H
