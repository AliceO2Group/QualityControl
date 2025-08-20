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
#include <optional>
#include <string>
#include <map>
// ROOT
#include <Rtypes.h>
#include <TObject.h>
// O2
#include <Common/Exceptions.h>
// QC
#include "QualityControl/Activity.h"

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
  MonitorObject(TObject* object, const std::string& taskName, const std::string& taskClass, const std::string& detectorName, int runNumber = 0, const std::string& periodName = "", const std::string& passName = "", const std::string& provenance = "qc");
  /// Destructor
  ~MonitorObject() override;

  // /// Copy constructor
  MonitorObject(const MonitorObject& other);
  /// Move constructor
  MonitorObject(MonitorObject&& other) /*noexcept*/ = default;
  /// Copy assignment operator
  MonitorObject& operator=(const MonitorObject& other);
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
  std::string getFullName() const;

  TObject* getObject() const;
  void setObject(TObject* object);

  bool isIsOwner() const;
  void setIsOwner(bool isOwner);

  const std::string& getTaskName() const;
  void setTaskName(const std::string& taskName);

  const std::string& getDetectorName() const;
  void setDetectorName(const std::string& detectorName);

  const std::string& getTaskClass() const;
  void setTaskClass(const std::string& taskClass);

  Activity& getActivity();
  const Activity& getActivity() const;
  void setActivity(const Activity& activity);
  void updateActivity(int runNumber, const std::string& periodName, const std::string& passName, const std::string& provenance);

  void setValidity(ValidityInterval);
  void updateValidity(validity_time_t value);
  ValidityInterval getValidity() const;

  void setCreateMovingWindow(bool);
  bool getCreateMovingWindow() const;

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
  /// \brief Get metadata value of given key, returns std::nullopt if none exists;
  std::optional<std::string> getMetadata(const std::string& key);

  /// \brief Check if the encapsulated object inherits from the given class name
  /// \param className Name of the class to check inheritance from
  /// \return true if the encapsulated object inherits from the given class, false otherwise
  bool encapsulatedInheritsFrom(std::string_view className) const;

  void Draw(Option_t* option) override;
  TObject* DrawClone(Option_t* option) const override;

  void Copy(TObject& object) const override;

  /// \brief Build the path to this object.
  /// Build the path to this object as it will appear in the GUI.
  /// \return A string containing the path.
  std::string getPath() const;

  const std::string& getDescription() const;
  void setDescription(const std::string& description);

 private:
  std::unique_ptr<TObject> mObject;
  std::string mTaskName;
  std::string mTaskClass;
  std::string mDetectorName;
  std::map<std::string, std::string> mUserMetadata;
  std::string mDescription;
  Activity mActivity;

  // indicates that we are the owner of mObject. It is the case by default. It is not the case when a task creates the
  // object.
  // TODO : maybe we should always be the owner ?
  bool mIsOwner;
  // tells Merger to create an object with data from the last cycle only on the side of the complete object
  bool mCreateMovingWindow = false;

  void releaseObject();
  void cloneAndSetObject(const MonitorObject&);

  ClassDefOverride(MonitorObject, 14);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_MONITOROBJECT_H
