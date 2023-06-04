// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QUALITYCONTROL_COMMONSPEC_H
#define QUALITYCONTROL_COMMONSPEC_H

///
/// \file   CommonSpec.h
/// \author Piotr Konopka
///

#include <string>
#include <cstdint>
#include <unordered_map>
#include "QualityControl/DiscardFileParameters.h"

namespace o2::quality_control::core
{

struct CommonSpec {
  CommonSpec() = default;

  std::unordered_map<std::string, std::string> database;
  int activityNumber{};
  int activityType{};
  std::string activityPeriodName;
  std::string activityPassName;
  std::string activityProvenance = "qc";
  uint64_t activityStart = 0;
  uint64_t activityEnd = -1;
  std::string monitoringUrl = "infologger:///debug?qc";
  std::string consulUrl;
  std::string conditionDBUrl = "http://ccdb-test.cern.ch:8080";
  DiscardFileParameters infologgerDiscardParameters;
  double postprocessingPeriod = 10.0;
  std::string bookkeepingUrl;
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_COMMONSPEC_H
