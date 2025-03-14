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

#ifndef QUALITYCONTROL_CONDITIONACCESS_H
#define QUALITYCONTROL_CONDITIONACCESS_H

#include <string>
#include <map>
#include <CCDB/BasicCCDBManager.h>

namespace o2::quality_control::core
{

class ConditionAccess
{
 public:
  /// Default constructor
  ConditionAccess() = default;
  /// Destructor
  virtual ~ConditionAccess() = default;

  void setCcdbUrl(const std::string& url)
  {
    o2::ccdb::BasicCCDBManager::instance().setURL(url);
    o2::ccdb::BasicCCDBManager::instance().setFatalWhenNull(false);
  }

  /**
   * Get an object from the CCDB. The object is owned by the CCDBManager, don't delete it !
   */
  template <typename T>
  T* retrieveConditionAny(std::string const& path, std::map<std::string, std::string> const& metadata = {}, long timestamp = -1);
};

template <typename T>
T* ConditionAccess::retrieveConditionAny(std::string const& path, std::map<std::string, std::string> const& metadata, long timestamp)
{
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(timestamp);
  return mgr.getSpecific<T>(path, mgr.getTimestamp(), metadata);
}

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_CONDITIONACCESS_H
