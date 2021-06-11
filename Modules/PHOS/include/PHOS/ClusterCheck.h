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
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 protected:
  static constexpr int kDeadThreshold = 10;   /// Number of new dead channels per module to decalre bad
  static constexpr int kNoisyThreshold = 2;   /// Number of new noisy channels per module to send warning
  static constexpr int kMaxUccupancyCut = 10; /// occupancy in noisy channel wrt mean over module

  std::unique_ptr<o2::phos::BadChannelsMap> mBadMap; /// bad map
  ClassDefOverride(ClusterCheck, 1);
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_PHOSCLUSTERCHECK_H
