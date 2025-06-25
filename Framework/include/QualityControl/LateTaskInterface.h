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
/// \file   LateTaskInterface.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_LATETASKINTERFACE_H
#define QC_CORE_LATETASKINTERFACE_H

#include <memory>
// O2
#include <Framework/InitContext.h>
#include <Framework/ProcessingContext.h>
// QC
#include "QualityControl/Activity.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/UserCodeInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"

namespace o2::monitoring
{
class Monitoring;
}

//namespace o2::globaltracking
//{
//struct DataRequest;
//}

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
/// TODO: to allow for more structured configuration, i see no other choice than providing the user with the late task config tree.
///   CustomParameters do not support tree-like structures. one could consider extending it, but i'm not sure if we can be fully backwards compatible.
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

  // Definition of the methods for the template method pattern
  virtual void initialize(o2::framework::InitContext& ctx) = 0;
  virtual void startOfActivity(const Activity& activity) = 0;
  // todo:
  //   we could come up with a dedicated QC data interface which supports our data sources.
  //   similarly to InputRecord, it could provide a facade to MOs and QOs cached by us in our internal format and expose
  //   methods to check if a requested resource is there, to get it, and to iterate over all available resources.
  //   optionally, it could also hide DPL's InputRecord or decorate it with a method which allows to access sampled
  //   and unsampled data in a unified way.
  // virtual void process(std::map<std::string, std::shared_ptr<const core::MonitorObject>>& moMap, std::map<std::string, std::shared_ptr<const core::QualityObject>>& qoMap) = 0;
  virtual void process(o2::framework::ProcessingContext& ctx) = 0;
  virtual void endOfActivity(const Activity& activity) = 0;
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
