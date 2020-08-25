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
/// \file   RepoPathUtils.h
/// \author Barthelemy von Haller
///

#ifndef QC_REPOPATH_UTILS_H
#define QC_REPOPATH_UTILS_H

#include <string>
#include <vector>
#include <Common/Exceptions.h>
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"

namespace o2::quality_control::core
{
class RepoPathUtils
{
 public:
  /**
   * Compute and return the path to the MonitorObject.
   * Current algorithm does qc/<detectorCode>/MO/<taskName>/<moName>
   * @param detectorCode
   * @param taskName
   * @param moName
   * @return the path to the MonitorObject
   */
  static std::string getMoPath(const std::string& detectorCode,
                               const std::string& taskName,
                               const std::string& moName)
  {
    std::string path = "qc/" + detectorCode + "/MO/" + taskName + "/" + moName;
    return path;
  }

  /**
   * Compute and return the path to the MonitorObject.
   * Current algorithm does qc/<detectorCode>/MO/<taskName>/<moName>
   * @param mo
   * @return the path to the MonitorObject
   */
  static std::string getMoPath(const MonitorObject* mo)
  {
    return getMoPath(mo->getDetectorName(), mo->getTaskName(), mo->getName());
  }

  /**
   * Compute and return the path to the QualityObject.
   * Current algorithm does  qc/<detectorCode>/QO/<checkName>[/<moName>].
   * The last, optional, part depends on policyName and uses the first element of the vector monitorObjectsNames.
   * @param detectorCode
   * @param checkName
   * @param policyName
   * @param monitorObjectsNames
   * @return the path to the QualityObject
   */
  static std::string getQoPath(const std::string& detectorCode,
                               const std::string& checkName,
                               const std::string& policyName = "",
                               const std::vector<std::string>& monitorObjectsNames = std::vector<std::string>())
  {
    std::string path = "qc/" + detectorCode + "/QO/" + checkName;
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
   * Current algorithm does  qc/<detectorCode>/QO/<checkName>[/<moName>].
   * The last, optional, part depends on policyName and uses the first element of the vector monitorObjectsNames.    
   * @param qo
   * @return the path to the QualityObject
   */
  static std::string getQoPath(const QualityObject* qo)
  {
    return getQoPath(qo->getDetectorName(), qo->getCheckName(), qo->getPolicyName(), qo->getMonitorObjectsNames());
  }
};
} // namespace o2::quality_control::core

#endif // QC_REPOPATH_UTILS_H
