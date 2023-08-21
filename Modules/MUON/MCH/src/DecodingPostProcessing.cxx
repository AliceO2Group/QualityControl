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
/// \file    DecodingPostProcessing.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Trending of the MCH tracking
/// \since   21/06/2022
///

#include "MCH/DecodingPostProcessing.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/PostProcessingConfigMCH.h"
#include <MCHMappingInterface/Segmentation.h>
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/DatabaseInterface.h"
#include <TH2.h>
#include <TDatime.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::muon;
using namespace o2::quality_control_modules::muonchambers;
using namespace o2::mch::raw;

void DecodingPostProcessing::configure(const boost::property_tree::ptree& config)
{
  PostProcessingConfigMCH mchConfig(getID(), config);

  mRefTimeStamp = mchConfig.getParameter<int64_t>("ReferenceTimeStamp", -1);
  auto refDate = mchConfig.getParameter<std::string>("ReferenceDate");
  if (!refDate.empty()) {
    TDatime date(refDate.c_str());
    mRefTimeStamp = date.Convert();
  }
  ILOG(Info, Devel) << "reference time stamp: " << mRefTimeStamp << "  (" << refDate << ")"
                    << AliceO2::InfoLogger::InfoLogger::endm;

  mFullHistos = mchConfig.getParameter<bool>("FullHistos", false);

  mCcdbObjects.emplace(errorsSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(hbPacketsSourceName(), CcdbObjectHelper());
  mCcdbObjects.emplace(syncStatusSourceName(), CcdbObjectHelper());

  for (auto source : mchConfig.dataSources) {
    std::string sourceType, sourceName;
    splitDataSourceName(source.name, sourceType, sourceName);
    if (sourceType.empty()) {
      continue;
    }

    auto obj = mCcdbObjects.find(sourceType);
    if (obj != mCcdbObjects.end()) {
      obj->second.mPath = source.path;
      obj->second.mName = sourceName;
    }
  }
}

//_________________________________________________________________________________________

void DecodingPostProcessing::createDecodingErrorsHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(errorsSourceName());
  if (obj != mCcdbObjects.end()) {
    mErrorsOnCycle = std::make_unique<HistoOnCycle<MergeableTH2Ratio>>();
  }

  //----------------------------------
  // Decoding errors plotters
  //----------------------------------

  mErrorsPlotter = std::make_unique<DecodingErrorsPlotter>("DecodingErrors/");
  mErrorsPlotter->publish(getObjectsManager());

  mErrorsPlotterOnCycle = std::make_unique<DecodingErrorsPlotter>("DecodingErrors/LastCycle/");
  mErrorsPlotterOnCycle->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void DecodingPostProcessing::createHeartBeatPacketsHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(hbPacketsSourceName());
  if (obj != mCcdbObjects.end()) {
    mHBPacketsOnCycle = std::make_unique<HistoOnCycle<MergeableTH2Ratio>>();
  }

  //----------------------------------
  // HeartBeat packets plotters
  //----------------------------------

  mHBPacketsPlotter = std::make_unique<HeartBeatPacketsPlotter>("HeartBeatPackets/", mFullHistos);
  mHBPacketsPlotter->publish(getObjectsManager());

  mHBPacketsPlotterOnCycle = std::make_unique<HeartBeatPacketsPlotter>("HeartBeatPackets/LastCycle/", mFullHistos);
  mHBPacketsPlotterOnCycle->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void DecodingPostProcessing::createSyncStatusHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  //------------------------------------------
  // Helpers to extract plots from last cycle
  //------------------------------------------

  auto obj = mCcdbObjects.find(syncStatusSourceName());
  if (obj != mCcdbObjects.end()) {
    mSyncStatusOnCycle = std::make_unique<HistoOnCycle<MergeableTH2Ratio>>();
  }

  //----------------------------------
  // Sync status  plotters
  //----------------------------------

  mSyncStatusPlotter = std::make_unique<FECSyncStatusPlotter>("SyncErrors/");
  mSyncStatusPlotter->publish(getObjectsManager());

  mSyncStatusPlotterOnCycle = std::make_unique<FECSyncStatusPlotter>("SyncErrors/LastCycle/");
  mSyncStatusPlotterOnCycle->publish(getObjectsManager());
}

//_________________________________________________________________________________________

void DecodingPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  createDecodingErrorsHistos(t, &qcdb);
  createHeartBeatPacketsHistos(t, &qcdb);
  createSyncStatusHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void DecodingPostProcessing::updateDecodingErrorsHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(errorsSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    MergeableTH2Ratio* hr = obj->second.get<MergeableTH2Ratio>();
    if (hr) {
      mErrorsPlotter->update(hr);
      //  extract the average occupancies on the last cycle
      mErrorsOnCycle->update(hr);
      mErrorsPlotterOnCycle->update(mErrorsOnCycle.get());
    }
  }
}

//_________________________________________________________________________________________

void DecodingPostProcessing::updateHeartBeatPacketsHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(hbPacketsSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    MergeableTH2Ratio* hr = obj->second.get<MergeableTH2Ratio>();
    if (hr) {
      mHBPacketsPlotter->update(hr);
      // extract the average occupancies on the last cycle
      mHBPacketsOnCycle->update(hr);
      mHBPacketsPlotterOnCycle->update(mHBPacketsOnCycle.get());
    }
  }
}

//_________________________________________________________________________________________

void DecodingPostProcessing::updateSyncStatusHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  auto obj = mCcdbObjects.find(syncStatusSourceName());
  if (obj != mCcdbObjects.end() && obj->second.update(qcdb, t.timestamp, t.activity)) {
    TH2F* hr = obj->second.get<MergeableTH2Ratio>();
    if (hr) {
      mSyncStatusPlotter->update(hr);
      // extract the average occupancies on the last cycle
      mSyncStatusOnCycle->update(hr);
      mSyncStatusPlotterOnCycle->update(mSyncStatusOnCycle.get());
    }
  }
}

//_________________________________________________________________________________________

void DecodingPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateDecodingErrorsHistos(t, &qcdb);
  updateHeartBeatPacketsHistos(t, &qcdb);
  updateSyncStatusHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void DecodingPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
