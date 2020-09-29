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
/// \file   PostProcessingInterface.cxx
/// \author Piotr Konopka
///

#include "QualityControl/PostProcessingInterface.h"

namespace o2::quality_control::postprocessing
{

std::string PostProcessingInterface::getName() const { return mName; }

void PostProcessingInterface::setName(const std::string& name)
{
  mName = name;
}

void PostProcessingInterface::configure(std::string name, const boost::property_tree::ptree& /*config*/)
{
  mName = name;
}

void PostProcessingInterface::setObjectsManager(std::shared_ptr<core::ObjectsManager> objectsManager)
{
  mObjectsManager = objectsManager;
}

std::shared_ptr<core::ObjectsManager> PostProcessingInterface::getObjectsManager()
{
  return mObjectsManager;
}

} // namespace o2::quality_control::postprocessing