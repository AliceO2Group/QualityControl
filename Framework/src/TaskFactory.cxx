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
/// \file   TaskFactory.cxx
/// \author Barthelemy von Haller
///
#include "QualityControl/TaskFactory.h"

#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TClass.h>
#include <TROOT.h>
#include <TSystem.h>

namespace o2::quality_control::core
{

TaskInterface* TaskFactory::create(TaskConfig& taskConfig, std::shared_ptr<ObjectsManager> objectsManager)
{
  TaskInterface* result = nullptr;
  QcInfoLogger& logger = QcInfoLogger::GetInstance();

  // Load the library
  std::string library = "lib" + taskConfig.moduleName;
  logger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
  int libLoaded = gSystem->Load(library.c_str(), "", true);
  if (libLoaded < 0) {
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
  result = static_cast<TaskInterface*>(cl->New());
  if (!result) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  result->setName(taskConfig.taskName);
  result->setObjectsManager(objectsManager);
  result->setCustomParameters(taskConfig.customParameters);
  result->setCcdbHost(taskConfig.conditionUrl);
  logger << "QualityControl Module " << taskConfig.moduleName << " loaded " << AliceO2::InfoLogger::InfoLogger::endm;

  return result;
}

} // namespace o2::quality_control::core
