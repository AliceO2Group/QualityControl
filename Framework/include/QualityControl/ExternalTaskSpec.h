#include <utility>

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

#ifndef QUALITYCONTROL_EXTERNALTASKSPEC_H
#define QUALITYCONTROL_EXTERNALTASKSPEC_H

///
/// \file   ExternalTaskSpec.h
/// \author Piotr Konopka
///

namespace o2::quality_control::core
{

struct ExternalTaskSpec {
  ExternalTaskSpec() = default;

  ExternalTaskSpec(std::string taskName, std::string query, bool active = true) : taskName(std::move(taskName)), query(std::move(query)), active(active)
  {
  }

  std::string taskName;
  std::string query;
  bool active = true;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_EXTERNALTASKSPEC_H
