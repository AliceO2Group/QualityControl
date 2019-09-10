// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "QualityControl/PostProcessingConfig.h"
#include <Configuration/ConfigurationInterface.h>

namespace o2::quality_control::postprocessing
{

PostProcessingConfig::PostProcessingConfig(std::string name, configuration::ConfigurationInterface& config) //
  : taskName(name),
    moduleName(config.get<std::string>("qc.postprocessing." + name + ".moduleName")),
    className(config.get<std::string>("qc.postprocessing." + name + ".className")),
    customParameters(config.getRecursiveMap("qc.postprocessing." + name + ".taskParameters"))
{
}

} // namespace o2::quality_control::postprocessing