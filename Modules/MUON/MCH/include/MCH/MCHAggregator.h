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
/// \file   MCHAggregator.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_MCH_MCHAGGREGATOR_H
#define QUALITYCONTROL_MCH_MCHAGGREGATOR_H

#include "QualityControl/AggregatorInterface.h"

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Aggregator for the MCH quality flags
/// \author Andrea Ferrero
class MCHAggregator : public o2::quality_control::checker::AggregatorInterface
{
 public:
  MCHAggregator() = default;
  void configure() override;
  std::map<std::string, o2::quality_control::core::Quality> aggregate(o2::quality_control::core::QualityObjectsMapType& qoMap) override;

  ClassDefOverride(MCHAggregator, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QUALITYCONTROL_MCH_MCHAGGREGATOR_H
