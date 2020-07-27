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
/// \file   CheckCompressedData.h
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw compressed data for TOF
///

#ifndef QC_MODULE_TOF_CHECKCOMPRESSEDDATA_H
#define QC_MODULE_TOF_CHECKCOMPRESSEDDATA_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::tof
{

/// \brief  Checker for the data produced by the TOF compressor (i.e. checking raw data)
///
/// \author Nicolo' Jacazio
class CheckCompressedData : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckCompressedData();
  /// Destructor
  ~CheckCompressedData() override;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

  ClassDefOverride(CheckCompressedData, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_CHECKCOMPRESSEDDATA_H
