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
/// \file   MonitorObject.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_MONITOROBJECT_H
#define QC_CORE_MONITOROBJECT_H

// std
#include <string>
#include <map>
// ROOT
#include <Rtypes.h>
#include <TObject.h>
// O2
#include <Common/Exceptions.h>

namespace o2::quality_control::core
{

struct DuplicateObjectError : virtual AliceO2::Common::ExceptionBase {
  const char* what() const noexcept override
  {
    return "Duplicate object error";
  }
};

/// \brief  This class keeps the meta data about one published object.
///
/// \author Barthelemy von Haller
class MonitorObject : public TObject
{
 public:
  /// Default constructor
  MonitorObject();
  MonitorObject(TObject* object, const std::string& taskName, const std::string& detectorName = "DET", int runNumber = 0);
  /// Destructor
  ~MonitorObject() override;

  /// Copy constructor
  MonitorObject(const MonitorObject& other) = default;
  /// Move constructor
  MonitorObject(MonitorObject&& other) /*noexcept*/ = default;
  /// Copy assignment operator
  MonitorObject& operator=(const MonitorObject& other) = default;
  /// Move assignment operator
  MonitorObject& operator=(MonitorObject&& other) /*noexcept*/ = default;

  /// \brief Return the name of the encapsulated object (if any).
  /// @return The name of the encapsulated object or "" if there is no object.
  const std::string getName() const;

  /// \brief Overwrite the TObject's method just to avoid confusion.
  /// @return The name of the encapsulated object or "" if there is no object.
  const char* GetName() const override;

  /// \brief Return joined task name and name of the encapsulated object (if any).
  /// @return The name as "{getTaskName()}/{getName())}.
  const std::string getFullName() const { return getTaskName() + "/" + getName(); }

  TObject* getObject() const { return mObject; }

  void setObject(TObject* object) { mObject = object; }

  bool isIsOwner() const { return mIsOwner; }

  void setIsOwner(bool isOwner) { mIsOwner = isOwner; }

  const std::string& getTaskName() const { return mTaskName; }
  void setTaskName(const std::string& taskName) { mTaskName = taskName; }

  const std::string& getDetectorName() const { return mDetectorName; }
  void setDetectorName(const std::string& detectorName) { mDetectorName = detectorName; }

  int getRunNumber() const;
  void setRunNumber(int runNumber);

  /// \brief Add key value pair that will end up in the database as metadata of the object
  /// Add a metadata (key value pair) to the MonitorObject. It will be stored in the database as metadata.
  /// If the key already exists the value will NOT be updated.
  void addMetadata(std::string key, std::string value);
  /// \brief Add key value pairs that will end up in the database as metadata of the object
  /// Add all the key-value pairs in the map to the MonitorObject. It will be stored in the database as metadata.
  /// If a key already exists the value will NOT be updated.
  void addMetadata(std::map<std::string, std::string> pairs);
  /// \brief Update the value of metadata.
  /// If the key does not exist it will ignore it.
  void updateMetadata(std::string key, std::string value);
  /// \brief Get the full map of user's metadata
  const std::map<std::string, std::string>& getMetadataMap() const;
  /// \brief Update the value of metadata or add it if it does not exist yet.
  void addOrUpdateMetadata(std::string key, std::string value);

  void Draw(Option_t* option) override;
  TObject* DrawClone(Option_t* option) const override;

  /// \brief Build the path to this object.
  /// Build the path to this object as it will appear in the GUI.
  /// \return A string containing the path.
  std::string getPath() const;

  const std::string& getDescription() const;
  void setDescription(const std::string& description);

 private:
  TObject* mObject;
  std::string mTaskName;
  std::string mDetectorName;
  std::map<std::string, std::string> mUserMetadata;
  std::string mDescription;
  int mRunNumber;

  // indicates that we are the owner of mObject. It is the case by default. It is not the case when a task creates the
  // object.
  // TODO : maybe we should always be the owner ?
  bool mIsOwner;

  ClassDefOverride(MonitorObject, 8);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_MONITOROBJECT_H
