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
/// \file   CommonInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_COMMONINTERFACE_H
#define QUALITYCONTROL_COMMONINTERFACE_H

#include <unordered_map>
#include <Rtypes.h>
#include <CCDB/CcdbApi.h>

#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control::core
{

/// \brief  Common interface for Check and Task Interfaces.
///
/// \author Barthelemy von Haller
class CommonInterface
{
 public:
  /// Default constructor
  CommonInterface() = default;
  /// Destructor
  virtual ~CommonInterface() = default;

  void setCustomParameters(const std::unordered_map<std::string, std::string>& parameters);

  /// \brief Configure the aggregator based on its name.
  ///
  /// The configuration of the object can't be done in the constructor because
  /// ROOT needs an argument-less constructor when streaming it. Users can use
  /// this method to configure their object. The name might be used to ask the configuration system
  /// for specific parameters.
  /// It is called each time mCustomParameters is updated.
  ///
  /// \param name The name of the task/aggregator/check.
  virtual void configure() = 0;

  void loadCcdb();
  void setCcdbUrl(const std::string& url);
  const std::string& getName() const;
  void setName(const std::string& name);

  template <typename T>
  T* retrieveConditionAny(std::string const& path, std::map<std::string, std::string> const& metadata = {},
                          long timestamp = -1);

 protected:
  std::unordered_map<std::string, std::string> mCustomParameters;
  std::string mName;

 private:
  std::shared_ptr<o2::ccdb::CcdbApi> mCcdbApi;
  std::string mCcdbUrl; // we need to keep the url in addition to the ccdbapi because we don't initialize the latter before the first call

  ClassDef(CommonInterface, 1)
};

template <typename T>
T* CommonInterface::retrieveConditionAny(std::string const& path, std::map<std::string, std::string> const& metadata,
                                         long timestamp)
{
  if (!mCcdbApi) {
    loadCcdb();
  }

  return mCcdbApi->retrieveFromTFileAny<T>(path, metadata, timestamp);
}

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_COMMONINTERFACE_H
