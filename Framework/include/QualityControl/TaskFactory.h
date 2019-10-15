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
/// \file   TaskFactory.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKFACTORY_H
#define QC_CORE_TASKFACTORY_H

// STL
#include <memory>
// QC
#include "QualityControl/TaskConfig.h"

namespace o2::quality_control::core
{

class TaskInterface;
class ObjectsManager;

/// \brief Factory in charge of creating tasks
///
/// The factory needs a library name and a class name provided as an object of type TaskConfig.
/// The class loaded in the library must inherit from TaskInterface.
class TaskFactory
{
 public:
  TaskFactory() = default;
  virtual ~TaskFactory() = default;

  /// \brief Create a new instance of a TaskInterface.
  /// The TaskInterface actual class is decided based on the parameters passed.
  /// \todo make it static ?
  /// \author Barthelemy von Haller
  TaskInterface* create(TaskConfig& taskConfig, std::shared_ptr<ObjectsManager> objectsManager);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_TASKFACTORY_H
