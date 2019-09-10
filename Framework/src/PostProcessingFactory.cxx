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
/// \file   PostProcessingFactory.cxx
/// \author Piotr Konopka
///

#include "QualityControl/PostProcessingFactory.h"

#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TClass.h>
#include <TROOT.h>
#include <TSystem.h>

using namespace o2::quality_control::core;

namespace o2::quality_control::postprocessing {

PostProcessingInterface* PostProcessingFactory::create(const PostProcessingConfig& config)
{
  PostProcessingInterface* result = nullptr;
  QcInfoLogger& logger = QcInfoLogger::GetInstance();

  // Load the library
  std::string library = "lib" + config.moduleName;
  logger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
  int libLoaded = gSystem->Load(library.c_str(), "", true);
  if (libLoaded < 0) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
  }

  // Get the class and instantiate
  logger << "Loading class " << config.className << AliceO2::InfoLogger::InfoLogger::endm;
  TClass* cl = TClass::GetClass(config.className.c_str());
  std::string tempString("Failed to instantiate Quality Control Module");
  if (!cl) {
    tempString += " because no dictionary for class named \"";
    tempString += config.className;
    tempString += "\" could be retrieved";
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  logger << "Instantiating class " << config.className << " (" << cl << ")"
         << AliceO2::InfoLogger::InfoLogger::endm;
  result = static_cast<PostProcessingInterface*>(cl->New());
  if (!result) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
//  result->setName(taskConfig.taskName);
//  result->setObjectsManager(objectsManager);
//  result->setCustomParameters(taskConfig.customParameters);
  logger << "QualityControl Module " << config.moduleName << " loaded " << AliceO2::InfoLogger::InfoLogger::endm;
  return nullptr;
}
} // namespace o2::quality_control::postprocessing