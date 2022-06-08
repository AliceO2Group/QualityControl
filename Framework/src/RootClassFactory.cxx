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
/// \file   RootClassFactory.cxx
/// \author Piotr Konopka
///

#include "QualityControl/RootClassFactory.h"

namespace o2::quality_control::core::root_class_factory
{

void loadLibrary(const std::string& moduleName)
{
  // Load the library
  std::string library = bfs::path(moduleName).is_absolute() ? moduleName : "libO2" + moduleName;
  ILOG(Info, Devel) << "Loading library " << library << ENDM;
  int libLoaded = gSystem->Load(library.c_str(), "", true);
  if (libLoaded < 0) {
    BOOST_THROW_EXCEPTION(FatalException() << errinfo_details("Failed to load Detector Publisher Library"));
  }
}

} // namespace o2::quality_control::core::root_class_factory