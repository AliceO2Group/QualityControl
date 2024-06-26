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
/// \file   ReferenceComparatorUtils.h
/// \author Andrea Ferrero and Barthelemy von Haller
///

#ifndef QUALITYCONTROL_ReferenceComparatorUtils_H
#define QUALITYCONTROL_ReferenceComparatorUtils_H

#include <memory>
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Activity.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RepoPathUtils.h"

namespace o2::quality_control_modules::common
{

//_________________________________________________________________________________________
//
// Get the reference plot for a given MonitorObject path

static std::shared_ptr<quality_control::core::MonitorObject> getReferencePlot(quality_control::repository::DatabaseInterface* qcdb, std::string& fullPath, uint32_t referenceRun, quality_control::core::Activity activity)
{
  uint64_t timeStamp = 0;
  activity.mId = referenceRun;
  const auto filterMetadata = quality_control::core::activity_helpers::asDatabaseMetadata(activity, false);
  const auto objectValidity = qcdb->getLatestObjectValidity(activity.mProvenance + "/" + fullPath, filterMetadata);
  if (objectValidity.isValid()) {
    timeStamp = objectValidity.getMax() - 1;
  } else {
    ILOG(Warning, Devel) << "Could not find the object '" << fullPath << "' for run " << activity.mId << ENDM;
    return nullptr;
  }

  std::string path;
  std::string name;
  if (!o2::quality_control::core::RepoPathUtils::splitObjectPath(fullPath, path, name)) {
    return nullptr;
  }
  // retrieve MO from CCDB
  auto mo = qcdb->retrieveMO(path, name, timeStamp, activity);

  return mo;
}

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_ReferenceComparatorUtils_H
