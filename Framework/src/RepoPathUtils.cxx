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

#include "QualityControl/RepoPathUtils.h"
#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>

namespace o2::quality_control::core
{

std::string RepoPathUtils::getTrfcPath(const std::string& detectorCode,
                                       const std::string& trfcName,
                                       const std::string& provenance)
{
  return provenance + "/" + detectorCode + "/TRFC/" + trfcName;
}

std::string RepoPathUtils::getTrfcPath(const TimeRangeFlagCollection* trfc)
{
  return getTrfcPath(trfc->getDetector(), trfc->getName(), trfc->getProvenance());
}

bool RepoPathUtils::isProvenanceAllowed(const std::string& provenance)
{
  return provenance == "qc" || provenance == "qc_async" || provenance == "qc_mc";
}

} // namespace o2::quality_control::core
