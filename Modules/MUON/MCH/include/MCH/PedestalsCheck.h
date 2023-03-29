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
/// \file   PedestalsCheck.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MCH_PEDESTALSCHECK_H
#define QC_MODULE_MCH_PEDESTALSCHECK_H

#include "MCH/Helpers.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"

namespace o2::quality_control_modules::muonchambers
{

class PedestalsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PedestalsCheck() = default;
  /// Destructor
  ~PedestalsCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  /// Maximum number of bad detection elements for "good" quality status
  int mMaxBadST12{ 1 };
  int mMaxBadST345{ 3 };
  /// Maximum fraction of bad channels in one DE for "good" quality status
  float mMaxBadFractionPerDE{ 0.1 };
  /// Maximum fraction of empty channels in one DE for "good" quality status
  float mMaxEmptyFractionPerDE{ 0.1 };
  /// Minimum statistics per DE for "good" quality status
  float mMinStatisticsPerDE{ 1000 };
  /// Minimum value of the z-axis range for the pedestals plots
  double mPedestalsPlotScaleMin{ 40 };
  /// Maximum value of the z-axis range for the pedestals plots
  double mPedestalsPlotScaleMax{ 250 };
  /// Minimum value of the z-axis range for the noise plots
  double mNoisePlotScaleMin{ 0 };
  /// Maximum value of the z-axis range for the noise plots
  double mNoisePlotScaleMax{ 1.5 };

  Quality mQualityBadChannels;
  Quality mQualityEmptyChannels;
  Quality mQualityStatistics;
  std::vector<std::string> mErrorMessages;

  QualityChecker mQualityChecker;

  ClassDefOverride(PedestalsCheck, 4);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
