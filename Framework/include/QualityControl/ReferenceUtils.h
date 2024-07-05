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

#ifndef QUALITYCONTROL_ReferenceUtils_H
#define QUALITYCONTROL_ReferenceUtils_H

#include <memory>
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Activity.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RepoPathUtils.h"

namespace o2::quality_control::checker
{

//_________________________________________________________________________________________
//
// Get the reference plot for a given MonitorObject path

static std::shared_ptr<quality_control::core::MonitorObject> getReferencePlot(quality_control::repository::DatabaseInterface* qcdb, std::string& fullPath,
                                                                              core::Activity referenceActivity)
{
  auto [success, path, name] = o2::quality_control::core::RepoPathUtils::splitObjectPath(fullPath);
  if (!success) {
    return nullptr;
  }
  return qcdb->retrieveMO(path, name, repository::DatabaseInterface::Timestamp::Latest, referenceActivity);
}

} // namespace o2::quality_control::checker

#endif // QUALITYCONTROL_ReferenceUtils_H
