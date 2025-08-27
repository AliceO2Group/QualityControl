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
/// \file   DecodingCheck.h
/// \author Andrea Ferrero, Sebastien Perrin
///

#ifndef QC_MODULE_MCH_DECODINGCHECK_H
#define QC_MODULE_MCH_DECODINGCHECK_H

#include "MCH/Helpers.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"
#include <string>

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check if the occupancy on each pad is between the two specified values
///
/// \author Andrea Ferrero, Sebastien Perrin
class DecodingCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  DecodingCheck() = default;
  /// Destructor
  ~DecodingCheck() override = default;

  // Override interface
  void configure() override;
  void startOfActivity(const Activity& activity) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;

 private:
  std::string mGoodFracHistName{ "DecodingErrors/GoodBoardsFractionPerDE" };
  std::string mGoodFracPerSolarHistName{ "DecodingErrors/GoodBoardsFractionPerSolar" };
  std::string mSyncFracHistName{ "SyncErrors/SyncedBoardsFractionPerDE" };
  std::string mSyncFracPerSolarHistName{ "SyncErrors/SyncedBoardsFractionPerSolar" };
  std::string mGoodFracRefCompHistName{ "DecodingErrors/RefComp/GoodBoardsFractionPerDE" };
  std::string mSyncFracRefCompHistName{ "SyncErrors/RefComp/SyncedBoardsFractionPerDE" };
  std::string mGoodFracPerSolarRefCompHistName{ "DecodingErrors/RefComp/GoodBoardsFractionPerSolar" };
  std::string mSyncFracPerSolarRefCompHistName{ "SyncErrors/RefComp/SyncedBoardsFractionPerSolar" };
  int mMaxBadST12{ 2 };
  int mMaxBadST345{ 3 };
  double mMinGoodErrorFrac{ 0.9 };
  std::array<std::optional<double>, 5> mMinGoodErrorFracPerStation;
  double mMinGoodErrorFracRatio{ 0.9 };
  double mMinGoodErrorFracPerSolar{ 0.5 };
  double mMinGoodErrorFracRatioPerSolar{ 0.9 };

  double mMinGoodSyncFrac{ 0.9 };
  std::array<std::optional<double>, 5> mMinGoodSyncFracPerStation;
  double mMinGoodSyncFracRatio{ 0.9 };
  double mMinGoodSyncFracPerSolar{ 0.5 };
  double mMinGoodSyncFracRatioPerSolar{ 0.9 };

  double mMinHeartBeatRate{ 0 };
  double mMaxHeartBeatRate{ 2 };

  double mGoodFracRatioPlotRange{ 0.2 };
  double mGoodFracRatioPerSolarPlotRange{ 0.2 };
  double mSyncFracRatioPlotRange{ 0.2 };
  double mSyncFracRatioPerSolarPlotRange{ 0.2 };

  QualityChecker mQualityChecker;
  std::array<Quality, getNumSolar()> mSolarQuality;

  ClassDefOverride(DecodingCheck, 2);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DECODINGCHECK_H
