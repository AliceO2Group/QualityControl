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
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ObjectMetadataKeys.h"
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
}

//_________________________________________________________________________________________

void BigScreen::initialize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  mColors[Quality::Null.getName()] = kViolet - 6;
  mColors[Quality::Bad.getName()] = kRed;
  mColors[Quality::Medium.getName()] = kOrange - 3;
  mColors[Quality::Good.getName()] = kGreen + 2;

  float dx = 1.0 / mConfig.mNCols;
  float dy = 1.0 / mConfig.mNRows;
  float paveHalfWidth = 0.3 * dx;
  float paveHalfHeight = 0.3 * dy;

  int index = 0;
  // add the paves associated to each quality source
  for (auto source : mConfig.dataSources) {
    auto detName = source.detector;
    auto row = index / mConfig.mNCols;
    auto col = index % mConfig.mNCols;

    if (row >= mConfig.mNRows) {
      break;
    }
    // put first pave on top-left
    row = mConfig.mNRows - row - 1;

    // coordinates of the center of the pave area
    float xc = dx * (0.5 + col);
    float yc = dy * (0.5 + row) - 0.01;

    // coordinates of the pave corners
    float x1 = xc - paveHalfWidth;
    float x2 = xc + paveHalfWidth;
    float y1 = yc - paveHalfHeight;
    float y2 = yc + paveHalfHeight;

    // label coordinates
    float xl = x1;
    float yl = y2 + 0.01;

    // add pave associated to the detector
    mLabels[detName] = std::make_unique<TText>(xl, yl, detName.c_str());
    for (auto& l : mLabels) {
      l.second->SetNDC();
      l.second->SetTextAlign(11);
      l.second->SetTextSize(0.05);
      l.second->SetTextFont(42);
    }

    // add pave associated to the detector
    mPaves[detName] = std::make_unique<TPaveText>(x1, y1, x2, y2);
    mPaves[detName]->SetOption("");
    mPaves[detName]->SetLineWidth(mConfig.mBorderWidth);

    index += 1;
  }

  mCanvas = std::make_unique<TCanvas>("BigScreen", "QC Big Screen", 800, 600);
  getObjectsManager()->startPublishing(mCanvas.get());
}

//_________________________________________________________________________________________
// Helper function for retrieving a QualityObject from the QCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
// A non-null QO is returned in the first element of the pair if the QO is found in the QCDB
// The second element of the pair is set to true if the QO has a time stamp more recent than a user-supplied threshold

static std::pair<std::shared_ptr<QualityObject>, bool> getQO(repository::DatabaseInterface& qcdb, Trigger t, BigScreenConfig::DataSource& source, long notOlderThan, bool ignoreActivity)
{
  // retrieve MO from CCDB - do not associate to trigger activity if ignoreActivity is true
  auto qo = ignoreActivity ? qcdb.retrieveQO(source.path, t.timestamp, {}) : qcdb.retrieveQO(source.path, t.timestamp, t.activity);
  if (!qo) {
    return { nullptr, false };
  }
  // get the MO creation time stamp
  long timeStamp{ 0 };
  auto iter = qo->getMetadataMap().find(repository::metadata_keys::created);
  if (iter != qo->getMetadataMap().end()) {
    timeStamp = std::stol(iter->second);
  }

  long elapsed = static_cast<long>(t.timestamp) - timeStamp;
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

  // draw messages in canvas
  mCanvas->Clear();
  mCanvas->cd();

  for (auto source : mConfig.dataSources) {
    auto det = source.detector;
    auto pave = mPaves[det].get();
    if (!pave) {
      continue;
    }
    auto label = mLabels[det].get();
    if (!label) {
      continue;
    }

    int color = kGray;

    // retrieve QO from CCDB, in the form of a std::pair<std::shared_ptr<QualityObject>, bool>
    // a valid object is returned in the first element of the pair if the QO is found in the QCDB
    // the second element of the pair is set to true if the QO has a time stamp not older than the provided threshold
    long notOlderThanMs = (mConfig.mNotOlderThan >= 0) ? static_cast<long>(mConfig.mNotOlderThan) * 1000 : std::numeric_limits<long>::max();
    auto qo = getQO(qcdb, t, source, notOlderThanMs, (mConfig.mIgnoreActivity != 0));
    if (qo.first && qo.second) {
      auto qoValue = qo.first->getQuality().getName();
      color = mColors[qoValue];
      pave->SetFillColor(color);
      pave->Clear();
      pave->AddText((std::string(" ") + qoValue + std::string(" ")).c_str());
    }
    pave->Draw();
    label->Draw();
  }
}

//_________________________________________________________________________________________

void BigScreen::finalize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::common
