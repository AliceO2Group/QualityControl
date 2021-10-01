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

#ifndef QUALITYCONTROL_POSTPROCESSINGTASKSPEC_H
#define QUALITYCONTROL_POSTPROCESSINGTASKSPEC_H

///
/// \file   PostProcessingTaskSpec.h
/// \author Piotr Konopka
///

#include <boost/property_tree/ptree.hpp>
#include <string>
#include <vector>

namespace o2::quality_control::core
{

struct PostProcessingTaskSpec {
  // default, invalid spec
  PostProcessingTaskSpec() = default;

  // minimal valid spec
  PostProcessingTaskSpec(std::string taskName, std::string className, std::string moduleName, std::string detectorName)
    : taskName(std::move(taskName)),
      className(std::move(className)),
      moduleName(std::move(moduleName)),
      detectorName(std::move(detectorName))
  {
  }

  // basic
  std::string taskName = "Invalid";
  std::string className = "Invalid";
  std::string moduleName = "Invalid";
  std::string detectorName = "Invalid";
  std::vector<std::string> initTriggers;
  std::vector<std::string> updateTriggers;
  std::vector<std::string> stopTriggers;
  // advanced
  bool active = true;
  boost::property_tree::ptree tree = {}; // todo see if it can be avoided
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_POSTPROCESSINGTASKSPEC_H
