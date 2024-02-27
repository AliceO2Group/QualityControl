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
/// \file    ReferenceValidatorTask.h
/// \author  Andrea Ferrero
/// \brief   Post-processing task that generates a summary of the comparison between given plots and their references
///

#include "Common/ReferenceValidatorTask.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/ActivityHelpers.h"
#include <TDatime.h>
#include <TPaveText.h>
#include <chrono>
// ROOT
#include <TH1.h>
#include <TCanvas.h>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

static bool splitObjectPath(std::string fullPath, std::string& path, std::string& name)
{
  std::string delimiter = "/";
  std::string det;
  size_t pos = fullPath.rfind(delimiter);
  if (pos == std::string::npos) {
    return false;
  }
  path = fullPath.substr(0, pos);
  name = fullPath.substr(pos + 1);
  return true;
}

//_________________________________________________________________________________________
// Helper function for retrieving a MonitorObject from the QCDB, in the form of a std::pair<std::shared_ptr<MonitorObject>, bool>
// A non-null MO is returned in the first element of the pair if the MO is found in the QCDB
// The second element of the pair is set to true if the MO has a time stamp more recent than a user-supplied threshold

static std::pair<std::shared_ptr<MonitorObject>, bool> getMO(repository::DatabaseInterface& qcdb, std::string fullPath, Trigger t, long notOlderThan)
{
  // find the time-stamp of the most recent object matching the current activity
  // if ignoreActivity is true the activity matching criteria are not applied
  auto timestamp = t.timestamp;
  const auto filterMetadata = activity_helpers::asDatabaseMetadata(t.activity, false);
  const auto objectValidity = qcdb.getLatestObjectValidity(t.activity.mProvenance + "/" + fullPath, filterMetadata);
  if (objectValidity.isValid()) {
    timestamp = objectValidity.getMax() - 1;
  } else {
    ILOG(Warning, Devel) << "Could not find the object '" << fullPath << "' for activity " << t.activity << ENDM;
    return { nullptr, false };
  }

  std::string path;
  std::string name;
  if (!splitObjectPath(fullPath, path, name)) {
    return { nullptr, false };
  }
  // retrieve QO from CCDB - do not associate to trigger activity if ignoreActivity is true
  auto qo = qcdb.retrieveMO(path, name, timestamp, t.activity);
  if (!qo) {
    return { nullptr, false };
  }

  long elapsed = static_cast<long>(t.timestamp) - timestamp;
  // check if the object is not older than a given number of milliseconds
  if (elapsed > notOlderThan) {
    ILOG(Warning, Devel) << "Object '" << fullPath << "' for activity " << t.activity << " is too old: " << elapsed << " > " << notOlderThan << ENDM;
    return { qo, false };
  }

  return { qo, true };
}

//_________________________________________________________________________________________
//
// Compate a MonitorObject with the corresponding reference, using the appropriate checker method
// based on the actual object inheritance (TH1, TGraph or TCanvas)

static Quality compare(std::shared_ptr<MonitorObject> mo, MOCompMethod method, double threshold)
{
  if (!mo) {
    return Quality::Null;
  }

  auto object = mo->getObject();
  if (!object) {
    return Quality::Null;
  }

  // check if the MO contains an histogram derived from THxRatio
  if (auto histRatio = dynamic_cast<TH1FRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  } else if (auto histRatio = dynamic_cast<TH1DRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  } else if (auto histRatio = dynamic_cast<TH2FRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  } else if (auto histRatio = dynamic_cast<TH2DRatio*>(object)) {
    return compareTObjects(histRatio->getNum(), histRatio->getDen(), method, threshold);
  }

  return Quality::Null;
}

//_________________________________________________________________________________________
// Helper functions for retrieving configuration parameters as text strings

template <>
std::string ReferenceValidatorTask::getParameter(std::string parName, const std::string defaultValue, const o2::quality_control::core::Activity& activity)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

template <>
std::string ReferenceValidatorTask::getParameter(std::string parName, const std::string defaultValue)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

//_________________________________________________________________________________________

void ReferenceValidatorTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = ReferenceValidatorTaskConfig(getID(), config);
}

//_________________________________________________________________________________________

void ReferenceValidatorTask::initialize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  // reset all existing objects
  mSummaryCanvas.reset();
  mDetailsCanvas.clear();
  mDetailsText.clear();

  mPlotNames.clear();

  auto& qcdb = services.get<repository::DatabaseInterface>();

  mColors[Quality::Null.getName()] = kViolet - 6;
  mColors[Quality::Bad.getName()] = kRed;
  mColors[Quality::Medium.getName()] = kOrange - 3;
  mColors[Quality::Good.getName()] = kGreen + 2;

  mNRows = getParameter<int>("nRows", 4, t.activity);
  mNCols = getParameter<int>("nCols", 5, t.activity);
  mBorderWidth = getParameter<int>("borderWidth", 4, t.activity);
  mNotOlderThan = getParameter<int>("notOlderThan", 120, t.activity);
  auto compMethodStr = getParameter<std::string>("method", "Deviation", t.activity);
  if (compMethodStr == "Deviation") {
    mCompMethod = MOCompDeviation;
  }
  if (compMethodStr == "Chi2") {
    mCompMethod = MOCompChi2;
  }
  mCompThreshold = getParameter<double>("threshold", 0.1, t.activity);

  // canvas showing the overall comparison result for each group
  mSummaryCanvas = std::make_unique<BigScreenCanvas>("Summary", "Summary", mNRows, mNCols, mBorderWidth);
  getObjectsManager()->startPublishing(mSummaryCanvas.get());

  // load and initialize the input groups
  int index = 0;
  for (auto group : mConfig.dataGroups) {
    auto groupName = group.name;
    auto& plotVec = mPlotNames[groupName];
    for (auto path : group.objects) {
      auto fullPath = group.path + "/" + path;
      plotVec.push_back(fullPath);
    }

    mDetailsText[groupName] = std::make_unique<TPaveText>(.05, .1, .95, .8);
    mDetailsText[groupName]->SetTextAlign(kHAlignLeft + kVAlignTop);
    mDetailsText[groupName]->SetBorderSize(0);
    mDetailsText[groupName]->SetFillColor(kWhite);
    mDetailsCanvas[groupName] = std::make_unique<TCanvas>((groupName + "/Messages").c_str(), "Messages", 800, 600);
    getObjectsManager()->startPublishing(mDetailsCanvas[groupName].get());

    mSummaryCanvas->add(groupName, index);
    index += 1;
  }
}

//_________________________________________________________________________________________

void ReferenceValidatorTask::update(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  for (auto& [groupName, plotVec] : mPlotNames) {
    // TPaveText with details for the current group
    auto& pave = mDetailsText[groupName];
    pave->Clear();

    Quality groupQuality = Quality::Null;
    for (auto& plot : plotVec) {
      Quality quality = Quality::Null;

      // get object for current timestamp - age limit is converted to milliseconds
      auto object = getMO(qcdb, plot, t, mNotOlderThan * 1000);

      // if the object is valid, compare it with the reference
      if (object.second) {
        quality = compare(object.first, mCompMethod, mCompThreshold);
      }

      // update the aggregated quality for the group
      if (groupQuality == Quality::Null || quality.isWorseThan(groupQuality)) {
        groupQuality = quality;
      }

      // add the result to the group details
      // the name of the plot is shown in a different color depending on the result of the comparison
      TText* t = pave->AddText(plot.c_str());
      t->SetTextColor(mColors[quality.getName()]);
    }

    // draw the text in the canvas associated to the current group
    auto& canvas = mDetailsCanvas[groupName];
    canvas->Clear();
    canvas->cd();
    pave->Draw();

    // updare the current group in the summary canvas
    mSummaryCanvas->set(groupName, mColors[groupQuality.getName()], groupQuality.getName());
  }

  mSummaryCanvas->update();
}

//_________________________________________________________________________________________

void ReferenceValidatorTask::finalize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::common
