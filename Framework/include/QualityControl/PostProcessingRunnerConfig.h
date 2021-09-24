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
/// \file   PostProcessingRunner.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGRUNNERCONFIG_H
#define QUALITYCONTROL_POSTPROCESSINGRUNNERCONFIG_H

#include <unordered_map>
#include <string>
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::postprocessing
{

struct PostProcessingRunnerConfig {
  std::string taskName;
  std::unordered_map<std::string, std::string> database;
  std::string consulUrl{};
  bool infologgerFilterDiscardDebug = false;
  int infologgerDiscardLevel = 21;
  boost::property_tree::ptree configTree{};
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGRUNNERCONFIG_H