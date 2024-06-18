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
/// \file     CTPTrendingTask.cxx
/// \author   Lucia Anna Tarasovicova
///

#include "CTP/CTPTrendingTask.h"
#include "CTP/TH1ctpReductor.h"

#include <QualityControl/DatabaseInterface.h>
//#include <QualityControl/MonitorObject.h>
#include <QualityControl/RootClassFactory.h>
#include <QualityControl/QcInfoLogger.h>
#include <QualityControl/ActivityHelpers.h>
//#include <QualityControl/RepoPathUtils.h>
//#include <QualityControl/UserCodeInterface.h>
#include <CCDB/BasicCCDBManager.h>
#include <DataFormatsCTP/Configuration.h>
//#include <DataFormatsCTP/RunManager.h>

#include <TCanvas.h>
#include <TH1.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;
using namespace o2::ctp;

void CTPTrendingTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingConfigCTP(getID(), config);
}

void CTPTrendingTask::initCTP(Trigger& t)
{
  std::string run = std::to_string(t.activity.mId);
  // CTPRunManager::setCCDBHost("https://alice-ccdb.cern.ch");
  // mCTPconfig = CTPRunManager::getConfigFromCCDB(t.timestamp, run);
  // there is an interface to access the CCDB : https://github.com/AliceO2Group/QualityControl/blob/master/doc/Advanced.md#accessing-objects-in-ccdb
  // if possible use PostProcessingInterface::retrieveCondition()
  std::string CCDBHost = "https://alice-ccdb.cern.ch";
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setURL(CCDBHost);
  map<string, string> metadata; // can be empty
  metadata["runNumber"] = run;
  mCTPconfig = mgr.getSpecific<CTPConfiguration>(CCDBPathCTPConfig, t.timestamp, metadata);
  if (mCTPconfig == nullptr) {
    ILOG(Warning, Support) << "CTP Config not found for run:" << run << " timesamp " << t.timestamp << ENDM;
    return;
  }
  // get the indices of the classes we want to trend
  std::vector<ctp::CTPClass> ctpcls = mCTPconfig->getCTPClasses();
  std::vector<int> clslist = mCTPconfig->getTriggerClassList();
  for (size_t i = 0; i < clslist.size(); i++) {
    for (size_t j = 0; j < mNumOfClasses; j++) {
      if (ctpcls[i].name.find(mClassNames[j]) != std::string::npos) {
        mClassIndex[j] = ctpcls[i].descriptorIndex + 1;
        break;
      }
    }
  }

  // Preparing data structure of TTree
  for (const auto& source : mConfig.dataSources) {
    mReductors.emplace(source.name, root_class_factory::create<o2::quality_control_modules::ctp::TH1ctpReductor>(source.moduleName, source.reductorName));
  }
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("time", &mTime);
  for (const auto& [sourceName, reductor] : mReductors) {
    reductor->SetMTVXIndex(mClassIndex[0]);
    reductor->SetMVBAIndex(mClassIndex[1]);
    reductor->SetTVXDCMIndex(mClassIndex[2]);
    reductor->SetTVXEMCIndex(mClassIndex[3]);
    reductor->SetTVXPHOIndex(mClassIndex[4]);
    mTrend->Branch(sourceName.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
  }
  getObjectsManager()->startPublishing(mTrend.get());
  ILOG(Debug, Devel) << "Trending run : " << run << ENDM;
}
void CTPTrendingTask::initialize(Trigger t, framework::ServiceRegistryRef services)
{
}

void CTPTrendingTask::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  if (mCTPconfig == nullptr) {
    initCTP(t);
    if (mCTPconfig == nullptr) {
      return;
    }
  }
  trendValues(t, qcdb);
  generatePlots();
}

void CTPTrendingTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
  generatePlots();
}

void CTPTrendingTask::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  mTime = activity_helpers::isLegacyValidity(t.activity.mValidity)
            ? t.timestamp / 1000
            : t.activity.mValidity.getMax() / 1000; // ROOT expects seconds since epoch.
  mMetaData.runNumber = t.activity.mId;

  ILOG(Info, Support) << "time stamp from activity " << t.timestamp << ENDM;
  ILOG(Info, Support) << "run number from activity " << t.activity.mId << ENDM;
  bool inputMissing = false;

  for (auto& dataSource : mConfig.dataSources) {
    auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
    if (!mo) {
      ILOG(Info, Support) << "no MO object" << ENDM;
      continue;
    }
    TObject* obj = mo ? mo->getObject() : nullptr;
    if (!obj) {
      ILOG(Info, Support) << "inputs not found" << ENDM;
      inputMissing = true;
      return;
    }
    mReductors[dataSource.name]->update(obj);
  }

  if (!inputMissing) {
    mTrend->Fill();
  }
}

void CTPTrendingTask::generatePlots()
{
  if (mTrend == nullptr) {
    ILOG(Info, Support) << "The trend object is not there, won't generate any plots." << ENDM;
    return;
  }

  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, won't generate any plots." << ENDM;
    return;
  }

  int index = 0; // for keeping track what is trended:
                 // 0 <= index < 5 - absolute input rates are trended
                 // 5 <= index < 10 - absolute class rates are trended
                 // 10 <= index < 15 - input rate ratios are trended
                 // 15 <= index < 19 - class rate ratios are trended
  for (const auto& plot : mConfig.plots) {

    // Before we generate any new plots, we have to delete existing under the same names.
    // It seems that ROOT cannot handle an existence of two canvases with a common name in the same process.
    if (mPlots.count(plot.name)) {
      delete mPlots[plot.name];
      mPlots[plot.name] = nullptr;
    }

    if (index > 4 && index < 10 && mClassIndex[index - 5] == 65) { // if the class index == 65, this class is not defined in the config, so it won't be trended
      ILOG(Info, Support) << "Class " << mClassNames[index - 5] << " is not trended." << ENDM;
      index++;
      continue;
    }

    if (index > 13 && (mClassIndex[index - 13] == 65 || mClassIndex[0] == 65)) { // if the class index == 65, this class is not defined in the config, so it won't be trended
      ILOG(Info, Support) << "Class ratio " << mClassNames[index - 13] << " / " << mClassNames[0] << " is not trended." << ENDM;
      index++;
      continue;
    }

    auto* c = new TCanvas();
    ILOG(Info, Support) << plot.varexp << " " << plot.selection << " " << plot.option << ENDM;
    mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), plot.option.c_str());

    c->SetName(plot.name.c_str());

    if (auto histo = dynamic_cast<TH1*>(c->GetPrimitive("htemp"))) {
      if (index < 5) {
        histo->SetTitle(mInputNames[index].c_str());
      } else if (index < 10) {
        histo->SetTitle(mClassNames[index - 5].c_str());
      } else if (index < 14) {
        histo->SetTitle(Form("%s/%s", mInputNames[index - 9].c_str(), mInputNames[0].c_str()));
      } else {
        histo->SetTitle(Form("%s/%s", mClassNames[index - 13].c_str(), mClassNames[0].c_str()));
      }

      if (index < 10) {
        histo->GetYaxis()->SetTitle("rate [kHz]");
      } else {
        histo->GetYaxis()->SetTitle("rate ratio");
      }
      c->Update();

      if (plot.varexp.find(":time") != std::string::npos) {
        histo->GetXaxis()->SetTimeDisplay(1);
        // It deals with highly congested dates labels
        histo->GetXaxis()->SetNdivisions(505);
        // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
        histo->GetXaxis()->SetTimeOffset(0.0);
        histo->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
      } else if (plot.varexp.find(":meta.runNumber") != std::string::npos) {
        histo->GetXaxis()->SetNoExponent(true);
      }
      histo->BufferEmpty();

    } else {
      ILOG(Error, Devel) << "Could not get the htemp histogram of the plot '" << plot.name << "'." << ENDM;
    }

    mPlots[plot.name] = c;
    getObjectsManager()->startPublishing(c, PublicationPolicy::Once);

    index++;
  }
}
