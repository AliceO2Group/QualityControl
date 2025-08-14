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
/// \file   ClustersCheck.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MCH_CLUSTERSCHECK_H
#define QC_MODULE_MCH_CLUSTERSCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/Quality.h"
#include <string>

namespace o2::quality_control::core
{
class MonitorObject;
}

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check if the clusters sizes and tracks-clusters association is within expected limits
///
/// \author Andrea Ferrero
class ClustersCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ClustersCheck() = default;
  /// Destructor
  ~ClustersCheck() override = default;

  // Override interface
  void configure() override;
  void startOfActivity(const Activity& activity) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override; private:
  /// quality flag associated to each input plot
  std::unordered_map<std::string, Quality> mQualities;
  /// acceptable limits for the average number of clusters associated tracks
  double mMinClustersPerTrack{ 9 };
  double mMaxClustersPerTrack{ 11 };
  /// minimum acceptale number of clusters per track in each chamber
  double mMinClustersPerChamber{ 0.8 };
  /// vertical range of the cluster per chamber plot
  double mClustersPerChamberRangeMin{ 0.6 };
  double mClustersPerChamberRangeMax{ 1.2 };
  /// minimum acceptable value for the average cluster size per chamber
  double mMinClusterSize{ 4 };
  /// vertical range of the cluster size per chamber plot
  double mClusterSizeRangeMin{ 0 };
  double mClusterSizeRangeMax{ 10 };
  /// size of the marker showing the average number of clusters
  float mMarkerSize{ 1.2 };

  ClassDefOverride(ClustersCheck, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_CLUSTERSCHECK_H
