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
#include "RootClassFactory.h"

using namespace o2::quality_control::core;

namespace o2::quality_control::postprocessing
{

// todo: consider having a common helper for each class which loads tasks like below (QC tasks, checks, pp)
PostProcessingInterface* PostProcessingFactory::create(const PostProcessingConfig& config)
{
  return root_class_factory::create<PostProcessingInterface>(config.moduleName, config.className);
}

} // namespace o2::quality_control::postprocessing