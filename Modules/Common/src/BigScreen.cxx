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
#include "Common/Utils.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ActivityHelpers.h"
#include <CommonUtils/StringUtils.h>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

void BigScreen::configure(const boost::property_tree::ptree& config)
{
  mConfig = BigScreenConfig(getID(), config);
}

//_________________________________________________________________________________________

std::string getParameter(o2::quality_control::core::CustomParameters customParameters,
                         std::string parName,
                         const o2::quality_control::core::Activity& activity)
{
  std::string parValue;
  auto parOpt = customParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    parValue = parOpt.value();
  } else {
    parOpt = customParameters.atOptional(parName);
    if (parOpt.has_value()) {
      parValue = parOpt.value();
    }
  }
  return parValue;
}

void BigScreen::initialize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  int nRows = getFromExtendedConfig<int>(t.activity, mCustomParameters, "nRows", 1);
  int nCols = getFromExtendedConfig<int>(t.activity, mCustomParameters, "nCols", 1);
  int borderWidth = getFromExtendedConfig<int>(t.activity, mCustomParameters, "borderWidth", 5);
  int foregroundColor = getFromExtendedConfig<int>(t.activity, mCustomParameters, "foregroundColor", 1);
  int backgroundColor = getFromExtendedConfig<int>(t.activity, mCustomParameters, "backgroundColor", 0);

  mMaxObjectTimeShift = getFromExtendedConfig<int>(t.activity, mCustomParameters, "maxObjectTimeShift", mMaxObjectTimeShift);
  mIgnoreActivity = getFromExtendedConfig<bool>(t.activity, mCustomParameters, "maxObjectTimeShift", mIgnoreActivity);

  auto labels = o2::utils::Str::tokenize(getFromExtendedConfig<std::string>(t.activity, mCustomParameters, "labels"), ',', false, false);
  if (labels.size() > (nRows * nCols)) {
    ILOG(Warning, Support) << "Number of labels larger than nRows*nCols, some labels will not be displayed correctly" << ENDM;
  }

  mCanvas.reset();
  mCanvas = std::make_unique<BigScreenCanvas>("BigScreen", "QC Big Screen", nRows, nCols, borderWidth, foregroundColor, backgroundColor);

  int index = 0;
  // add the paves associated to each quality source
  for (auto label : labels) {
    if (!label.empty()) {
      mCanvas->addBox(label, index);
    }
    index += 1;
  }

  getObjectsManager()->startPublishing(mCanvas.get(), PublicationPolicy::ThroughStop);
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
    // long notOlderThanMs = (mConfig.mNotOlderThan >= 0) ? static_cast<long>(mConfig.mNotOlderThan) * 1000 : std::numeric_limits<long>::max();
    auto qo = getQO(qcdb, t, source, mMaxObjectTimeShift * 1000, mIgnoreActivity);
    if (qo.first) {
      if (qo.second) {
        mCanvas->setQuality(source.detector, qo.first->getQuality());
      } else {
        mCanvas->setText(source.detector, kYellow, "Old");
      }
    } else {
      mCanvas->setText(source.detector, kGray, "NF");
    }
  }
  mCanvas->update();
}

//_________________________________________________________________________________________

void BigScreen::finalize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::common
