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
#include <iostream>
#include <map>
// ROOT
#include <TObject.h>
// QC
#include "QualityControl/Quality.h"

namespace o2::quality_control::core
{

/// \brief Container for the definition of a check
struct CheckDefinition {
  CheckDefinition() : result(Quality::Null) {}

  std::string name;
  std::string className;
  std::string libraryName;
  Quality result;
};

/// \brief  This class keeps the metadata about one published object.
///
/// \author Barthelemy von Haller
class MonitorObject : public TObject
{
 public:
  /// Default constructor
  MonitorObject();
  MonitorObject(TObject* object, const std::string& taskName);

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
  ///        One should rather use getName().
  const char* GetName() const override { return getName().c_str(); }

  const std::string& getTaskName() const { return mTaskName; }

  void setTaskName(const std::string& taskName) { mTaskName = taskName; }

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

  void Draw(Option_t* option) override;
  TObject* DrawClone(Option_t* option) const override;

 private:
  TObject* mObject;
  std::map<std::string /*checkName*/, CheckDefinition> mChecks;
  std::string mTaskName;

  // indicates that we are the owner of mObject. It is the case by default. It is not the case when a task creates the
  // object.
  // TODO : maybe we should always be the owner ?
  bool mIsOwner;

  ClassDefOverride(MonitorObject, 3);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_MONITOROBJECT_H
