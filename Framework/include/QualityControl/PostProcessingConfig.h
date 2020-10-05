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
/// \file   PostProcessingConfig.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGCONFIG_H
#define QUALITYCONTROL_POSTPROCESSINGCONFIG_H

#include <vector>
#include <string>
#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::quality_control::postprocessing
{

//todo pretty print

/// \brief  Post-processing configuration structure
struct PostProcessingConfig {
  PostProcessingConfig() = default;
  PostProcessingConfig(std::string name, const boost::property_tree::ptree& config);
  ~PostProcessingConfig() = default;
  std::string taskName = "";
  std::string moduleName = "";
  std::string className = "";
  std::string detectorName = "MISC";
  std::vector<std::string> initTriggers = {};
  std::vector<std::string> updateTriggers = {};
  std::vector<std::string> stopTriggers = {};
  std::string qcdbUrl = "";
  std::string ccdbUrl = "";
  std::string consulUrl = "";
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGCONFIG_H
