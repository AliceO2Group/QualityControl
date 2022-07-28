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
/// \file   WorstOfAllAggregator.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_WORSTOFALLAGGREGATOR_H
#define QUALITYCONTROL_WORSTOFALLAGGREGATOR_H

// ROOT
#include <Rtypes.h>
// QC
#include "QualityControl/AggregatorInterface.h"

namespace o2::quality_control_modules::common
{

/// \brief  Aggregator which selects the worst Quality and adds all FlagReasons
/// \author Piotr Konopka
class WorstOfAllAggregator : public o2::quality_control::checker::AggregatorInterface
{
 public:
  // Override interface
  void configure() override;
  std::map<std::string, o2::quality_control::core::Quality>
    aggregate(o2::quality_control::core::QualityObjectsMapType& qoMap) override;

  ClassDefOverride(WorstOfAllAggregator, 1);
};

} // namespace o2::quality_control_modules::common

#endif //QUALITYCONTROL_WORSTOFALLAGGREGATOR_H
