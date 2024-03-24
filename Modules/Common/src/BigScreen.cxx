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
/// \file    BigScreen.h
/// \author  Andrea Ferrero
/// \brief   Quality post-processing task that generates a canvas showing the aggregated quality of each system
///

#include "Common/BigScreen.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ActivityHelpers.h"
#include <TDatime.h>
#include <TPaveText.h>
#include <chrono>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

void BigScreen::configure(const boost::property_tree::ptree& config)
{
  mConfig = BigScreenConfig(getID(), config);

  mCanvas = std::make_unique<BigScreenCanvas>("BigScreen", "QC Big Screen", mConfig.mNRows, mConfig.mNCols, mConfig.mBorderWidth);

  int index = 0;
  // add the paves associated to each quality source
  for (auto source : mConfig.dataSources) {
    auto detName = source.detector;
    mCanvas->add(detName, index);
    index += 1;
  }
}

//_________________________________________________________________________________________

void BigScreen::initialize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  mCanvas->Clear();
  getObjectsManager()->startPublishing(mCanvas.get());
}

//_________________________________________________________________________________________
// Helper function for retrieving a QualityObject from the QCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
// A non-null QO is returned in the first element of the pair if the QO is found in the QCDB
// The second element of the pair is set to true if the QO has a time stamp more recent than a user-supplied threshold

static std::pair<std::shared_ptr<QualityObject>, bool> getQO(repository::DatabaseInterface& qcdb, Trigger t, BigScreenConfig::DataSource& source, long notOlderThan, bool ignoreActivity)
{
  // find the time-stamp of the most recent object matching the current activity
  // if ignoreActivity is true the activity matching criteria are not applied
  Activity activity = ignoreActivity ? Activity{} : t.activity;
  auto timestamp = t.timestamp;
  const auto objFullPath = t.activity.mProvenance + "/" + source.path;
  const auto filterMetadata = activity_helpers::asDatabaseMetadata(activity, false);
  const auto objectValidity = qcdb.getLatestObjectValidity(objFullPath, filterMetadata);
  if (objectValidity.isValid()) {
    timestamp = objectValidity.getMax() - 1;
  } else {
    ILOG(Info, Support) << "Could not find an object '" << objFullPath << "' for activity " << activity << ENDM;
    return { nullptr, false };
  }

  // retrieve QO from CCDB - do not associate to trigger activity if ignoreActivity is true
  auto qo = qcdb.retrieveQO(source.path, timestamp, activity);
  if (!qo) {
    return { nullptr, false };
  }

  long elapsed = static_cast<long>(t.timestamp) - timestamp;
  // check if the object is not older than a given number of milliseconds
  if (elapsed > notOlderThan) {
    return { qo, false };
  }

  return { qo, true };
}

//_________________________________________________________________________________________

void BigScreen::update(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  for (auto source : mConfig.dataSources) {
    // retrieve QO from CCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
    // a valid object is returned in the first element of the pair if the QO is found in the QCDB
    // the second element of the pair is set to true if the QO has a time stamp not older than the provided threshold
    long notOlderThanMs = (mConfig.mNotOlderThan >= 0) ? static_cast<long>(mConfig.mNotOlderThan) * 1000 : std::numeric_limits<long>::max();
    auto qo = getQO(qcdb, t, source, notOlderThanMs, (mConfig.mIgnoreActivity != 0));
    if (qo.first) {
      if (qo.second) {
        mCanvas->set(source.detector, qo.first->getQuality());
      } else {
        mCanvas->set(source.detector, kYellow, "Old");
      }
    } else {
      mCanvas->set(source.detector, kGray, "NF");
    }
  }
  mCanvas->update();
}

//_________________________________________________________________________________________

void BigScreen::finalize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::common
