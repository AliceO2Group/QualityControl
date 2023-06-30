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
#include <unordered_map>
#include <map>
#include <Rtypes.h>
#include <CCDB/BasicCCDBManager.h>

#include "QualityControl/CustomParameters.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control::core
{

/// \brief  Common interface for Check and Task Interfaces.
///
/// \author Barthelemy von Haller
class UserCodeInterface
{
 public:
  /// Default constructor
  UserCodeInterface() = default;
  /// Destructor
  virtual ~UserCodeInterface() = default;

  void setCustomParameters(const CustomParameters& parameters);

  /// \brief Configure the object.
  ///
  /// Users can use this method to configure their object.
  /// It is called each time mCustomParameters is updated, including the first time it is read.
  virtual void configure() = 0;

  void setCcdbUrl(const std::string& url);
  const std::string& getName() const;
  void setName(const std::string& name);

  /**
   * Get an object from the CCDB. The object is owned by the CCDBManager, don't delete it !
   */
  template <typename T>
  T* retrieveConditionAny(std::string const& path, std::map<std::string, std::string> const& metadata = {}, long timestamp = -1);

 protected:
  CustomParameters mCustomParameters;
  std::string mName;

  ClassDef(UserCodeInterface, 3)
};

template <typename T>
T* UserCodeInterface::retrieveConditionAny(std::string const& path, std::map<std::string, std::string> const& metadata, long timestamp)
{
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  o2::ccdb::BasicCCDBManager::instance().setTimestamp(timestamp);
  return mgr.getSpecific<T>(path, mgr.getTimestamp(), metadata);
}

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_USERCODEINTERFACE_H
