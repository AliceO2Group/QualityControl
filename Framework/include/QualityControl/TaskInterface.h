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
/// \file   TaskInterface.h
/// \author Piotr Konopka
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKINTERFACE_H
#define QC_CORE_TASKINTERFACE_H

#include <memory>
// O2
#include <Framework/InitContext.h>
#include <Framework/ProcessingContext.h>
// QC
#include "QualityControl/Activity.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/UserCodeInterface.h"

namespace o2::monitoring
{
class Monitoring;
}

namespace o2::globaltracking
{
struct DataRequest;
}

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
class TaskInterface : public UserCodeInterface
{
 public:
  /// \brief Constructor
  /// Can't be used when dynamically loading the class with ROOT.
  /// @param objectsManager
  explicit TaskInterface(ObjectsManager* objectsManager);

  /// \brief Default constructor
  TaskInterface() = default;

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
  virtual void startOfActivity(const Activity& activity) = 0;
  virtual void startOfCycle() = 0;
  virtual void monitorData(o2::framework::ProcessingContext& ctx) = 0;
  virtual void endOfCycle() = 0;
  virtual void endOfActivity(const Activity& activity) = 0;
  virtual void reset() = 0;

  /// \brief Called each time mCustomParameters is updated.
  virtual void configure() override;

  // Setters and getters
  void setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager);
  void setMonitoring(const std::shared_ptr<o2::monitoring::Monitoring>& mMonitoring);
  void setGlobalTrackingDataRequest(std::shared_ptr<o2::globaltracking::DataRequest>);
  const o2::globaltracking::DataRequest* getGlobalTrackingDataRequest() const;

 protected:
  std::shared_ptr<ObjectsManager> getObjectsManager();
  std::shared_ptr<o2::monitoring::Monitoring> mMonitoring;

 private:
  std::shared_ptr<ObjectsManager> mObjectsManager;
  std::shared_ptr<o2::globaltracking::DataRequest> mGlobalTrackingDataRequest;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKINTERFACE_H
