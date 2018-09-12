///
/// \file   TaskFactory.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_TASKFACTORY_H
#define QC_CORE_TASKFACTORY_H

#include <iostream>
#include <memory>
// ROOT
#include <TSystem.h>
#include <TClass.h>
#include <TROOT.h>
// O2
#include <Common/Exceptions.h>
#include "QualityControl/TaskConfig.h"
#include "QualityControl/TaskDevice.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2 {
namespace quality_control {
namespace core {

class TaskInterface;

class ObjectsManager;

/// \brief Factory in charge of creating tasks
///
/// The factory needs a library name and a class name provided as an object of type TaskConfig.
/// The class loaded in the library must inherit from TaskInterface.
class TaskFactory
{
 public:
  TaskFactory() {};
  virtual ~TaskFactory() {};

  using FatalException = AliceO2::Common::FatalException;
  using errinfo_details = AliceO2::Common::errinfo_details;

  /// \brief Create a new instance of a TaskInterface.
  /// The TaskInterface actual class is decided based on the parameters passed.
  /// \todo make it static ?
  /// \author Barthelemy von Haller
  template <class T>
  T* create(TaskConfig& taskConfig, std::shared_ptr <ObjectsManager> objectsManager)
  {
    T* result = nullptr;
    QcInfoLogger& logger = QcInfoLogger::GetInstance();

    // Load the library
    std::string library = "lib" + taskConfig.moduleName;
    logger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
    int libLoaded = gSystem->Load(library.c_str(), "", true);
    if (libLoaded) {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
    }

    // Get the class and instantiate
    logger << "Loading class " << taskConfig.className << AliceO2::InfoLogger::InfoLogger::endm;
    TClass* cl = TClass::GetClass(taskConfig.className.c_str());
    std::string tempString("Failed to instantiate Quality Control Module");
    if (!cl) {
      tempString += " because no dictionary for class named \"";
      tempString += taskConfig.className;
      tempString += "\" could be retrieved";
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
    }
    logger << "Instantiating class " << taskConfig.className << " (" << cl << ")"
           << AliceO2::InfoLogger::InfoLogger::endm;
    result = static_cast<T*>(cl->New());
    if (!result) {
      BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
    }
    result->setName(taskConfig.taskName);
    result->setObjectsManager(objectsManager);
    logger << "QualityControl Module " << taskConfig.moduleName << " loaded " << AliceO2::InfoLogger::InfoLogger::endm;

    return result;
  }
};

} // namespace core
} // namespace quality_control
} // namespace o2

#endif // QC_CORE_TASKFACTORY_H
