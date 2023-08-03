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
/// \file   RepoPathUtils.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOPATH_UTILS_H
#define QC_REPOPATH_UTILS_H

#include <string>
#include <vector>
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"
#include <Common/Exceptions.h>

namespace o2::quality_control
{
class TimeRangeFlagCollection;
}

namespace o2::quality_control::core
{

class RepoPathUtils
{
 public:
  /**
   * Compute and return the path to the MonitorObject.
   * Current algorithm does <provenance(qc)>/<detectorCode>/MO/<taskName>/<moName>
   * @param detectorCode
   * @param taskName
   * @param moName
   * @param provenance
   * @param includeProvenance
   * @return the path to the MonitorObject
   */
  static std::string getMoPath(const std::string& detectorCode,
                               const std::string& taskName,
                               const std::string& moName,
                               const std::string& provenance = "qc",
                               bool includeProvenance = true)
  {
    std::string path = (includeProvenance ? provenance + "/" : "") + detectorCode + "/MO/" + taskName + (moName.empty() ? "" : ("/" + moName));
    return path;
  }

  /**
   * Compute and return the path to the MonitorObject.
   * Current algorithm does <provenance(qc)>/<detectorCode>/MO/<taskName>/<moName>
   * @param mo
   * @param includeProvenance
   * @return the path to the MonitorObject
   */
  static std::string getMoPath(const MonitorObject* mo, bool includeProvenance = true)
  {
    return getMoPath(mo->getDetectorName(), mo->getTaskName(), mo->getName(), mo->getActivity().mProvenance, includeProvenance);
  }
  /**
   * Compute and return the path to the QualityObject.
   * Current algorithm does <provenance(qc)>/<detectorCode>/QO/<checkName>[/<moName>].
   * The last, optional, part depends on policyName and uses the first element of the vector monitorObjectsNames.
   * @param detectorCode
   * @param checkName
   * @param provenance
   * @param policyName
   * @param monitorObjectsNames
   * @param includeProvenance
   * @return the path to the QualityObject
   */
  static std::string getQoPath(const std::string& detectorCode,
                               const std::string& checkName,
                               const std::string& policyName = "",
                               const std::vector<std::string>& monitorObjectsNames = std::vector<std::string>(),
                               const std::string& provenance = "qc",
                               bool includeProvenance = true)
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

  /**
   * Compute and return the path to the QualityObject.
   * Current algorithm does <provenance(qc)>/<detectorCode>/QO/<checkName>[/<moName>].
   * The last, optional, part depends on policyName and uses the first element of the vector monitorObjectsNames.
   * @param qo
   * @return the path to the QualityObject
   */
  static std::string getQoPath(const QualityObject* qo, bool includeProvenance = true)
  {
    return getQoPath(qo->getDetectorName(),
                     qo->getCheckName(),
                     qo->getPolicyName(),
                     qo->getMonitorObjectsNames(),
                     qo->getActivity().mProvenance,
                     includeProvenance);
  }

  /**
   * Compute and return the path to the TRFCollection.
   * Current algorithm does <provenance(qc)>/<detectorCode>/TRFC/<trfcName>
   * @param detectorCode
   * @param trfcName
   * @param provenance
   * @return the path to the TRFCollection
   */
  static std::string getTrfcPath(const std::string& detectorCode,
                                 const std::string& trfcName,
                                 const std::string& provenance = "qc");
  /**
   * Compute and return the path to the TRFCollection.
   * Current algorithm does <provenance(qc)>/<detectorCode>/TRFC/<trfcName>
   * @param trfc
   * @return the path to the TRFCollection
   */
  static std::string getTrfcPath(const TimeRangeFlagCollection* trfc);

  static constexpr auto allowedProvenancesMessage = R"(Allowed provenances are "qc" (real data processed synchronously), "qc_async" (real data processed asynchronously) and "qc_mc" (simulated data).)";
  static bool isProvenanceAllowed(const std::string& provenance);
};
} // namespace o2::quality_control::core

#endif // QC_REPOPATH_UTILS_H
