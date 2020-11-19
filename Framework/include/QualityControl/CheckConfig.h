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
/// \file   CheckConfig.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_CHECKCONFIG_H
#define QC_CORE_CHECKCONFIG_H

#include <string>
#include <unordered_map>

namespace o2::quality_control::checker
{

/// \brief  Container for the configuration of a Check or an Aggregator.
struct CheckConfig {
  std::string name;
  std::string moduleName;
  std::string className;
  std::string detectorName = "MISC"; // intended to be the 3 letters code;
  std::unordered_map<std::string, std::string> customParameters = {};
  std::string policyType = "OnAny";
  std::vector<std::string> objectNames;
  bool allObjects = false;
};

} // namespace o2::quality_control::checker

#endif // QC_CORE_CHECKCONFIG_H
