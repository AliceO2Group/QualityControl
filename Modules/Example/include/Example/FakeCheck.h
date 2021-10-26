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
/// \file   FakeCheck.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_FAKECHECK_H
#define QC_MODULE_EXAMPLE_FAKECHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::example
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class FakeCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  FakeCheck() = default;
  /// Destructor
  ~FakeCheck() override = default;

  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(FakeCheck, 1);
};

} // namespace o2::quality_control_modules::example

#endif // QC_MODULE_EXAMPLE_FAKECHECK_H
