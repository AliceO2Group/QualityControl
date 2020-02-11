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
/// \file   NonEmpty.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_COMMON_NONEMPTY_H
#define QC_MODULE_COMMON_NONEMPTY_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::common
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class NonEmpty : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  NonEmpty() = default;
  /// Destructor
  ~NonEmpty() override = default;

  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  std::string getAcceptedType() override;

  ClassDefOverride(NonEmpty, 1);
};

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_COMMON_NONEMPTY_H
