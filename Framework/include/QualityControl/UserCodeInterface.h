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
/// \file   UserCodeInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_USERCODEINTERFACE_H
#define QUALITYCONTROL_USERCODEINTERFACE_H

#include <string>
#include <Rtypes.h>

#include "QualityControl/ConditionAccess.h"
#include "QualityControl/CustomParameters.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/CtpScalers.h"

namespace o2::quality_control::core
{
class UserCodeConfig;

/// \brief  Common interface for Check and Task Interfaces.
///
/// \author Barthelemy von Haller
class UserCodeInterface : public ConditionAccess
{
 public:
  /// Default constructor
  UserCodeInterface() = default;
  /// Destructor
  virtual ~UserCodeInterface() = default;

  /// \brief Configure the object.
  ///
  /// Users can use this method to configure their object.
  /// It is called each time mCustomParameters is updated, including the first time it is read.
  virtual void configure() = 0;

  const std::string& getName() const;
  void setName(const std::string& name);
  void setConfig(UserCodeConfig config);

 protected:
  /// \brief Call it to enable the retrieval of CTP scalers and use `getScalers` later
  void enableCtpScalers(size_t runNumber, std::string ccdbUrl);
  /// \brief Get the scalers's value for the given source
  double getScalersValue(std::string sourceName, size_t runNumber);

  void setDatabase(std::unordered_map<std::string, std::string> dbConfig);

  CustomParameters mCustomParameters;
  std::string mName;
  std::shared_ptr<repository::DatabaseInterface> mDatabase; //! the repository used by the Framework
  CtpScalers mCtpScalers;

  ClassDef(UserCodeInterface, 5)
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_USERCODEINTERFACE_H