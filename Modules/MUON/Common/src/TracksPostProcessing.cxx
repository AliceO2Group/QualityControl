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
/// \file    TracksPostProcessing.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MUON tracks
///

#include "MUONCommon/TracksPostProcessing.h"

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/CcdbDatabase.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/ActivityHelpers.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <TDatime.h>
#include <TH2.h>

using namespace o2::quality_control_modules::muon;
using namespace o2::quality_control::repository;

//_________________________________________________________________________________________
// Helper function for retrieving a MonitorObject from the QCDB, in the form of a std::pair<std::shared_ptr<MonitorObject>, bool>
// A non-null MO is returned in the first element of the pair if the MO is found in the QCDB
// The second element of the pair is set to true if the MO has a time stamp more recent than the last retrieved one

static std::pair<std::shared_ptr<MonitorObject>, bool> getMO(repository::DatabaseInterface& qcdb, std::string path, std::string name, Trigger t, uint64_t& timeStamp, int maxShiftMs = 1000 * 600)
{
  auto ts = t.timestamp;
  const auto objFullPath = t.activity.mProvenance + "/" + path + "/" + name;
  const auto filterMetadata = activity_helpers::asDatabaseMetadata(t.activity, false);
  const auto objectValidity = qcdb.getLatestObjectValidity(objFullPath, filterMetadata);
  if (objectValidity.isValid() && t.timestamp >= objectValidity.getMin() - maxShiftMs && t.timestamp <= objectValidity.getMax() + maxShiftMs) {
    ts = objectValidity.getMax() - 1;
  } else {
    ILOG(Warning, Devel) << "Could not find an object '" << objFullPath << "' in the proximity of timestamp "
                         << t.timestamp << " with max shift of " << maxShiftMs << "ms" << ENDM;
    return { nullptr, false };
  }

  // retrieve MO from CCDB
  auto mo = qcdb.retrieveMO(path, name, ts, t.activity);
  if (!mo) {
    return { nullptr, false };
  }
  // get the MO creation time stamp
  long newTimeStamp{ 0 };
  auto iter = mo->getMetadataMap().find(metadata_keys::created);
  if (iter != mo->getMetadataMap().end()) {
    newTimeStamp = std::stol(iter->second);
  }
  // check if the object is newer than the last visited one
  if (newTimeStamp <= timeStamp) {
    return { mo, false };
  }

  // update the time stamp of the last visited object
  timeStamp = newTimeStamp;

  // check if the object is not older than a given number of milliseconds
  long elapsed = static_cast<long>(t.timestamp) - ts;
  if (elapsed > maxShiftMs) {
    return { mo, false };
  }

  return { mo, true };
}

template <typename T>
T* getPlotFromMO(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  // Get ROOT object
  TObject* obj = mo ? mo->getObject() : nullptr;
  if (!obj) {
    return nullptr;
  }

  // Get histogram object
  T* h = dynamic_cast<T*>(obj);
  return h;
}

template <class HIST>
MatchingEfficiencyPlotter<HIST>::MatchingEfficiencyPlotter(std::string pathMatched, std::string pathMCH, std::string path, std::string plotName, int rebin)
  : MatchingEfficiencyPlotterInterface(), mRebin(rebin)
{
  mPlotPath[0] = pathMCH;
  mPlotName[0] = plotName;
  mPlotPath[1] = pathMatched;
  mPlotName[1] = plotName;

  mTimestamp[0] = 0;
  mTimestamp[1] = 0;

  mName = path + "/" + mPlotName[1];
}

template <class HIST>
void MatchingEfficiencyPlotter<HIST>::update(repository::DatabaseInterface& qcdb, Trigger t, std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager)
{
  std::string pathMCH = mPlotPath[0];
  auto moMCH = getMO(qcdb, pathMCH, mPlotName[0], t, mTimestamp[0]);
  std::string pathMatched = mPlotPath[1];
  auto moMatched = getMO(qcdb, pathMatched, mPlotName[1], t, mTimestamp[1]);
  // check if both MOs are existing and updated
  if (!moMCH.second || !moMatched.second) {
    return;
  }

  auto hMCH = getPlotFromMO<HIST>(moMCH.first);
  auto hMatched = getPlotFromMO<HIST>(moMatched.first);
  // check if both plots are valid
  if (!hMCH || !hMatched) {
    return;
  }

  TString name = mName.c_str();
  auto title = TString::Format("%s - matching eff.", hMatched->GetTitle());

  if (!mHistMatchingEff) {
    HIST* h = dynamic_cast<HIST*>(hMatched->Clone());
    if (!h) {
      return;
    }
    h->Rebin(mRebin);
    mHistMatchingEff.reset(h);
    mHistMatchingEff->Sumw2();
    mHistMatchingEff->SetStats(0);
    mHistMatchingEff->SetNameTitle(name, title);
    mHistMatchingEff->SetMarkerSize(0.25);
    mHistMatchingEff->SetLineColor(kBlack);
    std::string option;
    if (mHistMatchingEff->InheritsFrom(TH2::Class())) {
      option = "colz";
    } else {
      option = "PE";
    }
    histograms().emplace_back(HistInfo{ mHistMatchingEff.get(), option, "" });
    publish(objectsManager, histograms().back());
  }

  if (mRebin != 1) {
    HIST* h1 = dynamic_cast<HIST*>(hMatched->Clone());
    HIST* h2 = dynamic_cast<HIST*>(hMCH->Clone());
    h1->Rebin(mRebin);
    h2->Rebin(mRebin);
    mHistMatchingEff->Divide(h1, h2);
    delete h1;
    delete h2;
  } else {
    mHistMatchingEff->Divide(hMatched, hMCH);
  }
  mHistMatchingEff->SetNameTitle(name, title);
}

//_________________________________________________________________________________________

void TracksPostProcessing::createTrackHistos()
{
  for (auto& dataSource : mConfig->dataSources) {
    auto pathMatched = dataSource.plotsPath;
    auto pathRef = dataSource.refsPath;
    auto path = dataSource.outputPath;
    mMatchingEfficiencyPlotters.emplace_back(std::make_unique<MatchingEfficiencyPlotter<TH1>>(dataSource.plotsPath, dataSource.refsPath, dataSource.outputPath, dataSource.name, dataSource.rebin));
  }

  for (auto& plot : mMatchingEfficiencyPlotters) {
    plot->publish(getObjectsManager());
  }
}

//_________________________________________________________________________________________

void TracksPostProcessing::configure(const boost::property_tree::ptree& config)
{
  mConfig = std::make_unique<TracksPostProcessingConfig>(getID(), config);

  createTrackHistos();
}

//_________________________________________________________________________________________

void TracksPostProcessing::initialize(Trigger, framework::ServiceRegistryRef)
{
}

//_________________________________________________________________________________________

void TracksPostProcessing::updateTrackHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  for (auto& plot : mMatchingEfficiencyPlotters) {
    plot->update(*qcdb, t, getObjectsManager());
  }
}

//_________________________________________________________________________________________

void TracksPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateTrackHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void TracksPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
