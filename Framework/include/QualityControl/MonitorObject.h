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
// QC
#include "QualityControl/Quality.h"

namespace o2::quality_control::core
{

/// \brief Container for the definition of a check
struct CheckDefinition {
 public:
  CheckDefinition() : result(Quality::Null) {}
  virtual ~CheckDefinition() = default;

  std::string name;
  std::string className;
  std::string libraryName;
  Quality result;

  ClassDef(CheckDefinition, 1);
};

struct DuplicateObjectError : virtual AliceO2::Common::ExceptionBase {
  const char* what() const noexcept override
  {
    return "Duplicate object error";
  }
};

/// \brief  This class keeps the metadata about one published object.
///
/// \author Barthelemy von Haller
class MonitorObject : public TObject
{
 public:
  /// Default constructor
  MonitorObject();
  MonitorObject(TObject* object, const std::string& taskName, const std::string& detectorName = "DET");
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

  ///
  /// \brief Get the quality of this object.
  ///
  /// The method returns the lowest quality met amongst all the checks listed in \ref mChecks.
  /// If there are no checks, the method returns \ref Quality::Null.
  ///
  /// @return the quality of the object
  ///
  Quality getQuality() const;

  TObject* getObject() const { return mObject; }

  void setObject(TObject* object) { mObject = object; }

  std::map<std::string, CheckDefinition> getChecks() const { return mChecks; }

  bool isIsOwner() const { return mIsOwner; }

  void setIsOwner(bool isOwner) { mIsOwner = isOwner; }

  const std::string& getTaskName() const { return mTaskName; }
  void setTaskName(const std::string& taskName) { mTaskName = taskName; }

  const std::string& getDetectorName() const { return mDetectorName; }
  void setDetectorName(const std::string& detectorName) { mDetectorName = detectorName; }

  /// \brief Add a check to be executed on this object when computing the quality.
  /// If a check with the same name already exists it will be replaced by this check.
  /// Several checks can be added for the same check class name, but with different names (and
  /// they will get different configuration).
  /// \author Barthelemy von Haller
  /// \param name Arbitrary name to identify this Check.
  /// \param checkClassName The name of the class of the Check.
  /// \param checkLibraryName The name of the library containing the Check. If not specified it is taken from already
  /// loaded libraries.
  void addCheck(const std::string name, const std::string checkClassName, const std::string checkLibraryName = "");

  /// \brief Add or update the check with the provided name.
  /// @param checkName The name of the check. If another check has already been added with this name it will be
  /// replaced.
  /// @param check The check to add or replace.
  void addOrReplaceCheck(std::string checkName, CheckDefinition check);

  /// \brief Set the given quality to the check called checkName.
  /// If no check exists with this name, it throws a AliceO2::Common::ObjectNotFoundError.
  /// @param checkName The name of the check
  /// @param quality The new quality of the check.
  /// \throw AliceO2::Common::ObjectNotFoundError
  void setQualityForCheck(std::string checkName, Quality quality);

  /// Return the check (by value!) for the given name.
  /// If no such check exists, AliceO2::Common::ObjectNotFoundError is thrown.
  /// \param checkName The name of the check
  /// \return The CheckDefinition of the check named checkName.
  /// \throw AliceO2::Common::ObjectNotFoundError
  CheckDefinition getCheck(std::string checkName) const;

  /// \brief Add key value pair that will end up in the database
  /// Add a metadata (key value pair) to the MonitorObject. It will be stored in the database.
  /// If the key already exists the value will be updated.
  void addMetadata(std::string key, std::string value);

  std::map<std::string, std::string> getMetadataMap() const;

  void Draw(Option_t* option) override;
  TObject* DrawClone(Option_t* option) const override;

 private:
  TObject* mObject;
  std::map<std::string /*checkName*/, CheckDefinition> mChecks;
  std::string mTaskName;
  std::string mDetectorName;
  std::map<std::string, std::string> mUserMetadata;

  // indicates that we are the owner of mObject. It is the case by default. It is not the case when a task creates the
  // object.
  // TODO : maybe we should always be the owner ?
  bool mIsOwner;

  ClassDefOverride(MonitorObject, 5);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_MONITOROBJECT_H
