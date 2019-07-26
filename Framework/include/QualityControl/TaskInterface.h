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
/// \file   TaskInterface.h
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKINTERFACE_H
#define QC_CORE_TASKINTERFACE_H

#include <memory>
#include <string>
// fixes problem of ''assert' not declared in this scope' in Framework/InitContext.h.
// Maybe ROOT does some #undef assert?
#include <cassert>

// O2
#include <Framework/InitContext.h>
#include <Framework/ProcessingContext.h>
// Configuration
#include <Configuration/ConfigurationInterface.h>
// QC
#include "QualityControl/Activity.h"
#include "QualityControl/ObjectsManager.h"

namespace o2::quality_control::core
{

/// \brief  Skeleton of a QC task.
///
/// Purely abstract class defining the skeleton and the common interface of a QC task.
/// It is therefore the parent class of any QC task.
/// It is responsible for the instantiation, modification and destruction of the TObjects that are published.
///
/// It is a part of the template method design pattern.
///
/// \author Barthelemy von Haller
/// \author Piotr Konopka
class TaskInterface
{
 public:
  /// \brief Constructor
  /// Can't be used when dynamically loading the class with ROOT.
  /// @param objectsManager
  explicit TaskInterface(ObjectsManager* objectsManager);

  /// \brief Default constructor
  TaskInterface();

  /// \brief Destructor
  virtual ~TaskInterface() noexcept = default;
  /// Copy constructor
  TaskInterface(const TaskInterface& other) = default;
  /// Move constructor
  TaskInterface(TaskInterface&& other) noexcept = default;
  /// Copy assignment operator
  TaskInterface& operator=(const TaskInterface& other) = default;
  /// Move assignment operator
  TaskInterface& operator=(TaskInterface&& other) /* noexcept */ = default; // error with gcc if noexcept

  // Definition of the methods for the template method pattern
  virtual void initialize(o2::framework::InitContext& ctx) = 0;
  virtual void startOfActivity(Activity& activity) = 0;
  virtual void startOfCycle() = 0;
  virtual void monitorData(o2::framework::ProcessingContext& ctx) = 0;
  virtual void endOfCycle() = 0;
  virtual void endOfActivity(Activity& activity) = 0;
  virtual void reset() = 0;

  // Setters and getters
  void setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager);
  void setName(const std::string& name);
  void setCustomParameters(const o2::configuration::KeyValueMap& parameters);
  const std::string& getName() const;

 protected:
  std::shared_ptr<ObjectsManager> getObjectsManager();
  o2::configuration::KeyValueMap mCustomParameters;

 private:
  // TODO should we rather have a global/singleton for the objectsManager ?
  std::shared_ptr<ObjectsManager> mObjectsManager;
  std::string mName;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKINTERFACE_H
