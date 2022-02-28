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
/// \file   DummyDatabase.h
/// \author Piotr Konopka
///

#ifndef QC_REPOSITORY_DUMMYDATABASE_H
#define QC_REPOSITORY_DUMMYDATABASE_H

#include "QualityControl/DatabaseInterface.h"

namespace o2::quality_control::repository
{

/// \brief Dummy database which does nothing. Use it to avoid writing to QC repository.
class DummyDatabase : public DatabaseInterface
{
 public:
  DummyDatabase() = default;
  virtual ~DummyDatabase() = default;

  void connect(std::string host, std::string database, std::string username, std::string password) override;
  void connect(const std::unordered_map<std::string, std::string>& config) override;
  void storeAny(const void* obj, std::type_info const& typeInfo, std::string const& path, std::map<std::string, std::string> const& metadata,
                std::string const& detectorName, std::string const& taskName, long from = -1, long to = -1) override;
  // MonitorObject
  void storeMO(std::shared_ptr<const o2::quality_control::core::MonitorObject> q, long from, long to) override;
  std::shared_ptr<o2::quality_control::core::MonitorObject> retrieveMO(std::string taskName, std::string objectName, long timestamp = 0) override;
  // QualityObject
  void storeQO(std::shared_ptr<const o2::quality_control::core::QualityObject> q, long from, long to) override;
  std::shared_ptr<o2::quality_control::core::QualityObject> retrieveQO(std::string checkerName, long timestamp = 0) override;
  // TRFC
  void storeTRFC(std::shared_ptr<const o2::quality_control::TimeRangeFlagCollection> trfc) override;
  std::shared_ptr<o2::quality_control::TimeRangeFlagCollection> retrieveTRFC(const std::string& name, const std::string& detector, int runNumber = 0,
                                                                             const std::string& passName = "", const std::string& periodName = "",
                                                                             const std::string& provenance = "", long timestamp = -1) override;
  // General
  void* retrieveAny(std::type_info const& tinfo, std::string const& path,
                    std::map<std::string, std::string> const& metadata, long timestamp = -1,
                    std::map<std::string, std::string>* headers = nullptr,
                    const std::string& createdNotAfter = "", const std::string& createdNotBefore = "") override;
  std::string retrieveJson(std::string path, long timestamp, const std::map<std::string, std::string>& metadata) override;
  TObject* retrieveTObject(std::string path, const std::map<std::string, std::string>& metadata, long timestamp = -1, std::map<std::string, std::string>* headers = nullptr) override;

  void disconnect() override;
  void prepareTaskDataContainer(std::string taskName) override;
  std::vector<std::string> getPublishedObjectNames(std::string taskName) override;
  void truncate(std::string taskName, std::string objectName) override;
  void setMaxObjectSize(size_t maxObjectSize) override;

 private:
};

} // namespace o2::quality_control::repository

#endif // QC_REPOSITORY_DUMMYDATABASE_H
