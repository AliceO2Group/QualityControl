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
/// \file   SkeletonAggregator.h
/// \author My Name
///

#ifndef QUALITYCONTROL_SKELETONAGGREGATOR_H
#define QUALITYCONTROL_SKELETONAGGREGATOR_H

// ROOT
#include <Rtypes.h>
// QC
#include "QualityControl/AggregatorInterface.h"

namespace o2::quality_control_modules::skeleton
{

/// \brief  Example QC quality aggregator
/// \author My Name
class SkeletonAggregator: public o2::quality_control::checker::AggregatorInterface
{
 public:
  // Override interface
  void configure(std::string name) override;
  std::vector<Quality> aggregate(QualityObjectsType* qos) override;

 ClassDefOverride(SkeletonAggregator, 1);
};

} // namespace o2::quality_control_modules::skeleton

#endif //QUALITYCONTROL_SKELETONAGGREGATOR_H
