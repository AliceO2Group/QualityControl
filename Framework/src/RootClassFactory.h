// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//

///
/// \author Piotr Konopka
/// \file RootClassFactory.h
///
#ifndef QUALITYCONTROL_ROOTCLASSFACTORY_H
#define QUALITYCONTROL_ROOTCLASSFACTORY_H

#include <string>
// O2
#include <Common/Exceptions.h>
// ROOT
#include <TClass.h>
#include <TROOT.h>
#include <TSystem.h>
// Boost
#include <boost/filesystem/path.hpp>
// QC
#include "QualityControl/QcInfoLogger.h"

namespace bfs = boost::filesystem;

namespace o2::quality_control::core
{

namespace root_class_factory
{

using FatalException = AliceO2::Common::FatalException;
using errinfo_details = AliceO2::Common::errinfo_details;

template <typename T>
T* create(std::string moduleName, std::string className)
{
  T* result = nullptr;
  QcInfoLogger& logger = QcInfoLogger::GetInstance();

  // Load the library
  std::string library = bfs::path(moduleName).is_absolute() ? moduleName : "lib" + moduleName;
  logger << "Loading library " << library << AliceO2::InfoLogger::InfoLogger::endm;
  int libLoaded = gSystem->Load(library.c_str(), "", true);
  if (libLoaded < 0) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
  }

  // Get the class and instantiate
  logger << "Loading class " << className << AliceO2::InfoLogger::InfoLogger::endm;
  TClass* cl = TClass::GetClass(className.c_str());
  std::string tempString("Failed to instantiate Quality Control Module");
  if (!cl) {
    tempString += " because no dictionary for class named \"";
    tempString += className;
    tempString += "\" could be retrieved";
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  logger << "Instantiating class " << className << " (" << cl << ")"
         << AliceO2::InfoLogger::InfoLogger::endm;
  result = static_cast<T*>(cl->New());
  if (!result) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details(tempString));
  }
  logger << "QualityControl Module " << moduleName << " loaded " << AliceO2::InfoLogger::InfoLogger::endm;

  return result;
}

} // namespace root_class_factory

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_ROOTCLASSFACTORY_H
