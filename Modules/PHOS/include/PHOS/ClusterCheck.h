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
/// \file   ClusterCheck.h
/// \author Dmitri Peresunko
///

#ifndef QC_MODULE_PHOS_PHOSCLUSTERCHECK_H
#define QC_MODULE_PHOS_PHOSCLUSTERCHECK_H

#include "QualityControl/CheckInterface.h"
#include "DataFormatsPHOS/Cluster.h"
#include "DataFormatsPHOS/BadChannelsMap.h"
#include "PHOSBase/Geometry.h"

namespace o2::quality_control_modules::phos
{

/// \brief  Check mos: appearence of dead regions in occupancy plots, mean and RMS etc.
///
/// \author Dmitri Peresunko
class ClusterCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ClusterCheck() = default;
  /// Destructor
  ~ClusterCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 protected:
  int mDeadThreshold[5] = { 10, 10, 10, 10, 10 };   /// Number of new dead channels per module to decalre bad
  int mNoisyThreshold[5] = { 2, 2, 2, 2, 2 };       /// Number of new noisy channels per module to send warning
  int mMaxOccupancyCut[5] = { 10, 10, 10, 10, 10 }; /// occupancy in noisy channel wrt mean over module
  float mCluEnergyRangeL[5] = { 1., 1., 1., 1., 1. };
  float mCluEnergyRangeR[5] = { 10., 10., 10., 10., 10. };
  float mMinCluEnergyMean[5] = { 2., 2., 2., 2., 2 };
  float mMaxCluEnergyMean[5] = { 4., 4., 4., 4., 4 };

  const o2::phos::BadChannelsMap* mBadMap; /// bad map
  ClassDefOverride(ClusterCheck, 2);
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_PHOSCLUSTERCHECK_H
