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

#ifndef LATETASKFACTORY_H
#define LATETASKFACTORY_H

///
/// \file   LateTaskFactory.h
/// \author Piotr Konopka
///

#include <memory>

#include "QualityControl/LateTaskConfig.h"
#include "QualityControl/LateTaskInterface.h"
#include "QualityControl/RootClassFactory.h"

namespace o2::quality_control::core
{

class LateTaskInterface;
class ObjectsManager;

/// \brief Factory in charge of creating late tasks
///
/// The factory needs a library name and a class name provided as an object of type LateTaskConfig.
/// The class loaded in the library must inherit from LateTaskInterface.
class LateTaskFactory
{
  public:
  LateTaskFactory() = default;
  virtual ~LateTaskFactory() = default;

  /// \brief Create a new instance of a LateTaskInterface.
  /// The LateTaskInterface actual class is decided based on the parameters passed.
  static LateTaskInterface* create(const LateTaskConfig& taskConfig, std::shared_ptr<ObjectsManager> objectsManager)
  {
    auto* result = root_class_factory::create<LateTaskInterface>(taskConfig.moduleName, taskConfig.className);
    result->setName(taskConfig.name);
    result->setObjectsManager(objectsManager);
    result->setCustomParameters(taskConfig.customParameters);
    result->setCcdbUrl(taskConfig.ccdbUrl);

    return result;
  }
};

} // namespace o2::quality_control::core

#endif //LATETASKFACTORY_H
