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
/// \file   QualityCheck.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MCH_QUALITYCHECK_H
#define QC_MODULE_MCH_QUALITYCHECK_H

#include "MCH/Helpers.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"
#include <string>

using namespace o2::quality_control_modules::muon;
namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check the overall data quality
///
/// \author Andrea Ferrero
class QualityCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  QualityCheck() = default;
  /// Destructor
  ~QualityCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  std::string mQualityHistName{ "MCHQuality" };
  double mMinGoodFraction{ 0.5 };

  ClassDefOverride(QualityCheck, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_QUALITYCHECK_H
