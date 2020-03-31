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
  // MonitorObject
  void storeMO(std::shared_ptr<o2::quality_control::core::MonitorObject> q) override;
  std::shared_ptr<o2::quality_control::core::MonitorObject> retrieveMO(std::string taskName, std::string objectName, long timestamp = 0) override;
  std::string retrieveMOJson(std::string taskName, std::string objectName, long timestamp = 0) override;
  // QualityObject
  void storeQO(std::shared_ptr<o2::quality_control::core::QualityObject> q) override;
  std::shared_ptr<o2::quality_control::core::QualityObject> retrieveQO(std::string checkerName, long timestamp = 0) override;
  std::string retrieveQOJson(std::string checkName, long timestamp = 0) override;
  // General
  std::string retrieveJson(std::string path, long timestamp = 0) override;
  TObject* retrieveTObject(std::string path, long timestamp = -1, std::map<std::string, std::string>* headers = nullptr) override;

  void disconnect() override;
  void prepareTaskDataContainer(std::string taskName) override;
  std::vector<std::string> getPublishedObjectNames(std::string taskName) override;
  void truncate(std::string taskName, std::string objectName) override;

 private:
};

} // namespace o2::quality_control::repository

#endif // QC_REPOSITORY_DUMMYDATABASE_H
