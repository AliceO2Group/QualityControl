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
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"
#include <DataFormatsQualityControl/TimeRangeFlagCollection.h>
#include <Common/Exceptions.h>

namespace o2::quality_control::core
{

std::string RepoPathUtils::getMoPath(const std::string& detectorCode,
                                     const std::string& taskName,
                                     const std::string& moName,
                                     const std::string& provenance,
                                     bool includeProvenance)
{
  std::string path = (includeProvenance ? provenance + "/" : "") + detectorCode + "/MO/" + taskName + (moName.empty() ? "" : ("/" + moName));
  return path;
}

std::string RepoPathUtils::getMoPath(const MonitorObject* mo, bool includeProvenance)
{
  return getMoPath(mo->getDetectorName(), mo->getTaskName(), mo->getName(), mo->getActivity().mProvenance, includeProvenance);
}

std::string RepoPathUtils::getQoPath(const std::string& detectorCode,
                                     const std::string& checkName,
                                     const std::string& policyName,
                                     const std::vector<std::string>& monitorObjectsNames,
                                     const std::string& provenance,
                                     bool includeProvenance)
{
  std::string path = (includeProvenance ? provenance + "/" : "") + detectorCode + "/QO/" + checkName;
  if (policyName == "OnEachSeparately") {
    if (monitorObjectsNames.empty()) {
      BOOST_THROW_EXCEPTION(AliceO2::Common::FatalException() << AliceO2::Common::errinfo_details("getQoPath: The vector of monitorObjectsNames is empty."));
    }
    path += "/" + monitorObjectsNames[0];
  }
  return path;
}

std::string RepoPathUtils::getQoPath(const QualityObject* qo, bool includeProvenance)
{
  return getQoPath(qo->getDetectorName(),
                   qo->getCheckName(),
                   qo->getPolicyName(),
                   qo->getMonitorObjectsNames(),
                   qo->getActivity().mProvenance,
                   includeProvenance);
}

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
