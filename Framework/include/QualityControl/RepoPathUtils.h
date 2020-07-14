// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

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
   * @param mo
   * @return
   */
  static std::string getMoPath(const MonitorObject *mo) {
    std::string path = "qc/" + mo->getDetectorName() + "/" + mo->getTaskName() + "/" + mo->getName();
    return path;
  }

  /**
   * Compute and return the path to the QualityObject.
   * @param qo
   * @return
   */
  static std::string getQoPath(const QualityObject *qo) {
    std::string path = "qc/checks/" + qo->getDetectorName() + "/" + qo->getCheckName();
    if (qo->getPolicyName() == "OnEachSeparately") {
      std::vector<std::string> monitorObjectsNames = qo->getMonitorOjbectsNames();
      if (monitorObjectsNames.empty()) {
        BOOST_THROW_EXCEPTION(AliceO2::Common::FatalException() <<
                                                                AliceO2::Common::errinfo_details("getQoPath: The vector of monitorObjectsNames is empty."));
      }
      path += "/" + monitorObjectsNames[0];
    }
    return path;
  }
};
}

#endif // QC_REPOPATH_UTILS_H
