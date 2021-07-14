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
/// \file   EverIncreasingGraph.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_DAQ_EVERINCREASINGRAPH_H
#define QC_MODULE_DAQ_EVERINCREASINGRAPH_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::common
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class EverIncreasingGraph : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  EverIncreasingGraph() = default;
  /// Destructor
  ~EverIncreasingGraph() override = default;

  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

 private:
  ClassDefOverride(EverIncreasingGraph, 1);
};

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_DAQ_EVERINCREASINGRAPH_H
