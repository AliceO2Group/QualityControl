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
/// \file   CheckInterface.h
/// \author Barthelemy von Haller
///

#ifndef QC_CHECKER_CHECKINTERFACE_H
#define QC_CHECKER_CHECKINTERFACE_H

#include "QualityControl/Quality.h"
#include "QualityControl/UserCodeInterface.h"

namespace o2::quality_control::core
{
class MonitorObject;
}

// todo: do not expose other namespaces in headers
using namespace o2::quality_control::core;

namespace o2::quality_control::checker
{

/// \brief  Skeleton of a check.
///
/// \author Barthelemy von Haller
class CheckInterface : public UserCodeInterface
{
 public:
  /// Default constructor
  CheckInterface() = default;
  /// Destructor
  virtual ~CheckInterface() = default;

  /// \brief Returns the quality associated with these objects.
  ///
  /// @param moMap A map of the the MonitorObjects to check and their full names.
  /// @return The quality associated with these objects.
  virtual core::Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) = 0;

  /// \brief Modify the aspect of the plot.
  ///
  /// Modify the aspect of the plot.
  /// It is usually based on the result of the check (passed as quality)
  ///
  /// @param mo          The MonitorObject to beautify.
  /// @param checkResult The quality returned by the check. It is not the same as the quality of the mo
  ///                    as the latter represents the combination of all the checks the mo passed. This
  ///                    parameter is to be used to pass the result of the check of the same class.
  virtual void beautify(std::shared_ptr<core::MonitorObject> mo, core::Quality checkResult) = 0;

  /// \brief Returns the name of the class that can be treated by this check.
  ///
  /// The name of the class returned by this method will be checked against the MonitorObject's encapsulated
  /// object's class. If it is the same or a parent then the check will be applied. Therefore, this method
  /// must return the highest class in the hierarchy that this check can use.
  /// If the class does not override it, we return "TObject".
  ///
  /// \author Barthelemy von Haller
  virtual std::string getAcceptedType();

  bool isObjectCheckable(const std::shared_ptr<core::MonitorObject> mo);
  bool isObjectCheckable(const core::MonitorObject* mo);

 protected:
  /// \brief Called each time mCustomParameters is updated.
  virtual void configure() override;

  ClassDef(CheckInterface, 4)
};

} // namespace o2::quality_control::checker

#endif /* QC_CHECKER_CHECKINTERFACE_H */
