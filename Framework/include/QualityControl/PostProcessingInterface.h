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
/// \file   PostProcessingInterface.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINTERFACE_H
#define QUALITYCONTROL_POSTPROCESSINTERFACE_H

#include <string>
#include <Framework/ServiceRegistry.h>
#include <boost/property_tree/ptree_fwd.hpp>
#include "QualityControl/Triggers.h"
#include "QualityControl/ObjectsManager.h"

namespace o2::quality_control::postprocessing
{

/// \brief  Skeleton of a post-processing task.
///
/// Abstract class defining the skeleton and the common interface of a post-processing task.
/// It is therefore the parent class of any post-processing task.
/// It is responsible for retrieving, processing and storing the data, mainly from and to QC repository.
///
/// \author Piotr Konopka
class PostProcessingInterface
{
 public:
  PostProcessingInterface() = default;
  virtual ~PostProcessingInterface() = default;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param name     Name of the task
  /// \param config   ConfigurationInterface with prefix set to ""
  virtual void configure(std::string name, const boost::property_tree::ptree& config);

  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  virtual void initialize(Trigger trigger, framework::ServiceRegistry& services) = 0;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  virtual void update(Trigger trigger, framework::ServiceRegistry& services) = 0;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  virtual void finalize(Trigger trigger, framework::ServiceRegistry& services) = 0;

  void setObjectsManager(std::shared_ptr<core::ObjectsManager> objectsManager);
  void setName(const std::string& name);
  std::string getName() const;

 protected:
  std::shared_ptr<core::ObjectsManager> getObjectsManager();

 private:
  std::string mName;
  std::shared_ptr<core::ObjectsManager> mObjectsManager;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINTERFACE_H
