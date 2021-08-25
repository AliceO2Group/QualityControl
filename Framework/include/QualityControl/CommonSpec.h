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
#include <unordered_map>
#include <boost/property_tree/ptree_fwd.hpp>

namespace o2::quality_control::core
{

struct CommonSpec {
  CommonSpec() = default;

  std::unordered_map<std::string, std::string> database;
  int activityNumber;
  int activityType;
  std::string activityPeriodName;
  std::string activityPassType;
  std::string activityProvenance;
  std::string monitoringUrl = "infologger:///debug?qc";
  std::string consulUrl;
  std::string conditionDBUrl = "http://ccdb-test.cern.ch:8080";
  bool infologgerFilterDiscardDebug = false;
  int infologgerDiscardLevel = 21;

  std::string configurationSource;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_COMMONSPEC_H
