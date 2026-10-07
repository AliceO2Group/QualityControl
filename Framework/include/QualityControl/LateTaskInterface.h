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

#ifndef QC_CORE_LATETASKINTERFACE_H
#define QC_CORE_LATETASKINTERFACE_H

///
/// \file   LateTaskInterface.h
/// \author Piotr Konopka
///

#include <memory>

#include <Framework/InitContext.h>
#include <Framework/ProcessingContext.h>

#include "QualityControl/Activity.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/UserCodeInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/QCInputs.h"

namespace o2::monitoring
{
class Monitoring;
}

// namespace o2::globaltracking
//{
// struct DataRequest;
// }

namespace o2::framework
{
struct ConcreteDataMatcher;
}

namespace o2::quality_control::core
{

/// \brief  Skeleton of a late QC task.
///
/// Purely abstract class defining the skeleton and the common interface of a late QC task.
/// It is therefore the parent class of any late QC task.
/// It is responsible for the instantiation, modification and destruction of the TObjects that are published.
///
/// Late tasks can process any output of a Task, Check or Aggregator and produce new MonitorObjects.
/// In a multinode setup, they always run on remote (QC) nodes, so they can access merged MonitorObjects and any
/// QualityObjects. Thus, it is not possible to run late Tasks on FLPs or EPNs.
/// In async QC, late Tasks can be used in combination with a QCDB reader (not implemented yet) to perform
/// any trends or correlations on series of objects which are available only in the QCDB.
///
/// TODO: one could even consider allowing to feed late tasks with output of Reductors.
///   It could be an opportunity to refactor them as well (and rename them to Reducers, which sounds more like English).
/// TODO: think how to allow to produce new plots after each `process()` in sync, while producing just one at the end for async.
/// \author Piotr Konopka
class LateTaskInterface : public UserCodeInterface
{
 public:
  /// \brief Constructor
  /// Can't be used when dynamically loading the class with ROOT.
  /// @param objectsManager
  explicit LateTaskInterface(ObjectsManager* objectsManager);

  /// \brief Default constructor
  LateTaskInterface() = default;

  /// \brief Destructor
  virtual ~LateTaskInterface() noexcept = default;
  /// Copy constructor
  LateTaskInterface(const LateTaskInterface& other) = default;
  /// Move constructor
  LateTaskInterface(LateTaskInterface&& other) noexcept = default;
  /// Copy assignment operator
  LateTaskInterface& operator=(const LateTaskInterface& other) = default;
  /// Move assignment operator
  LateTaskInterface& operator=(LateTaskInterface&& other) /* noexcept */ = default; // error with gcc if noexcept

  /// Invoked during task initialization
  virtual void initialize(o2::framework::InitContext& ctx) = 0;
  /// Invoked at the start of run in synchronous mode and before the first `process()` in asynchronous mode
  virtual void startOfActivity(const Activity& activity) = 0;
  /// Invoked each time new data arrive
  virtual void process(const core::QCInputs& data) = 0;
  /// Invoked at the end of run in synchronous mode and after the last `process()` in asynchronous mode
  virtual void endOfActivity(const Activity& activity) = 0;
  /// Invoked at the reset() transition in synchronous mode and during workflow cleanup in asynchronous mode
  virtual void reset() = 0;

  /// \brief Called each time mCustomParameters is updated.
  virtual void configure() override;

  // Setters and getters
  void setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager);
  void setMonitoring(const std::shared_ptr<o2::monitoring::Monitoring>& mMonitoring);

 protected:
  std::shared_ptr<ObjectsManager> getObjectsManager();
  std::shared_ptr<o2::monitoring::Monitoring> mMonitoring;

 private:
  std::shared_ptr<ObjectsManager> mObjectsManager;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_LATETASKINTERFACE_H
