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
/// \file   TPCAggregator.h
/// \author Marcel Lesch
///

#ifndef QUALITYCONTROL_TPCAGGREGATOR_H
#define QUALITYCONTROL_TPCAGGREGATOR_H

// ROOT
#include <Rtypes.h>
// QC
#include "QualityControl/AggregatorInterface.h"

namespace o2::quality_control_modules::tpc
{

/// \brief  Aggregator which selects the worst Quality and aggregates MetaData of TPC QOs
/// \author Marcel Lesch
class TPCAggregator : public o2::quality_control::checker::AggregatorInterface
{
 public:
  // Override interface
  void configure() override;
  std::map<std::string, o2::quality_control::core::Quality>
    aggregate(o2::quality_control::core::QualityObjectsMapType& qoMap) override;

  ClassDefOverride(TPCAggregator, 1);

 private:
  void insertQOName(std::string& MetaData, std::string& insertTitle);
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_TPCAGGREGATOR_H
