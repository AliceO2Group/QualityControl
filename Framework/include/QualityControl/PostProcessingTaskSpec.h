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

namespace o2::quality_control::postprocessing
{

struct PostProcessingTaskSpec {
  // default, invalid spec
  PostProcessingTaskSpec() = default;

  // minimal valid spec
  PostProcessingTaskSpec(std::string taskName)
    : taskName(std::move(taskName))
  {
  }

  // Post Processing Tasks configure themselves with a ptree.
  // While this is a lack of consequence, with respect to other *Specs,
  // I am afraid it is too late to change it, since also users rely on this (see ITS Trending Task).

  std::string taskName = "Invalid";
  bool active = true;
  boost::property_tree::ptree tree = {};
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_POSTPROCESSINGTASKSPEC_H
