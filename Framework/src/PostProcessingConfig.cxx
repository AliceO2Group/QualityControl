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
/// \file   PostProcessingConfig.cxx
/// \author Piotr Konopka
///

#include "QualityControl/PostProcessingConfig.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::postprocessing
{

PostProcessingConfig::PostProcessingConfig(std::string name, const boost::property_tree::ptree& config) //
  : taskName(name),
    moduleName(config.get<std::string>("qc.postprocessing." + name + ".moduleName")),
    className(config.get<std::string>("qc.postprocessing." + name + ".className")),
    detectorName(config.get<std::string>("qc.postprocessing." + name + ".detectorName", "MISC")),
    qcdbUrl(config.get<std::string>("qc.config.database.implementation") == "CCDB" ? config.get<std::string>("qc.config.database.host") : ""),
    ccdbUrl(config.get<std::string>("qc.config.conditionDB.url", ""))
{
  for (const auto& initTrigger : config.get_child("qc.postprocessing." + name + ".initTrigger")) {
    initTriggers.push_back(initTrigger.second.get_value<std::string>());
  }
  for (const auto& updateTrigger : config.get_child("qc.postprocessing." + name + ".updateTrigger")) {
    updateTriggers.push_back(updateTrigger.second.get_value<std::string>());
  }
  for (const auto& stopTrigger : config.get_child("qc.postprocessing." + name + ".stopTrigger")) {
    stopTriggers.push_back(stopTrigger.second.get_value<std::string>());
  }
}

} // namespace o2::quality_control::postprocessing