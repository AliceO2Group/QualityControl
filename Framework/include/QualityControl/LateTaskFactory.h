//
// Created by pkonopka on 23/06/25.
//

#ifndef LATETASKFACTORY_H
#define LATETASKFACTORY_H

// STL
#include <memory>
// QC
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
