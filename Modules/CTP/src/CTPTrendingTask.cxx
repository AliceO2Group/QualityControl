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
#include <QualityControl/MonitorObject.h>
#include <QualityControl/RootClassFactory.h>
#include <QualityControl/QcInfoLogger.h>
#include <QualityControl/ActivityHelpers.h>
#include <QualityControl/RepoPathUtils.h>
#include <QualityControl/Reductor.h>
#include <QualityControl/UserCodeInterface.h>
#include <CCDB/BasicCCDBManager.h>
#include <DataFormatsCTP/Configuration.h>
#include <DataFormatsCTP/RunManager.h>

#include <TCanvas.h>
#include <TH1.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;
using namespace o2::quality_control_modules::ctp;
using namespace o2::ctp;

void CTPTrendingTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingConfigCTP(getID(), config);
}

void CTPTrendingTask::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // Preparing data structure of TTree
  for (const auto& source : mConfig.dataSources) {
    mReductors.emplace(source.name, root_class_factory::create<TH1ctpReductor>(source.moduleName, source.reductorName));
  }
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("time", &mTime);
  for (const auto& [sourceName, reductor] : mReductors) {
    mTrend->Branch(sourceName.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
  }

  getObjectsManager()->startPublishing(mTrend.get());
}

void CTPTrendingTask::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

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

  map<string, string> metadata; // can be empty

  std::string run = std::to_string(t.activity.mId);
  CTPRunManager::setCCDBHost("https://alice-ccdb.cern.ch");
  mCTPconfig = CTPRunManager::getConfigFromCCDB(t.timestamp, run);

  bool foundInputs = false;

  for (auto& dataSource : mConfig.dataSources) {
    auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
    if (!mo) {
      ILOG(Info, Support) << "no MO object" << ENDM;
      continue;
    }
    ILOG(Info, Support) << "Got MO " << mo << ENDM;
    TObject* obj = mo ? mo->getObject() : nullptr;
    if (obj) {
      foundInputs = true;
      mReductors[dataSource.name]->update(obj);
    }
  }

  if (!foundInputs) {
    ILOG(Info, Support) << "inputs not found" << ENDM;
    return;
  }

  mTrend->Fill();
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

  int index = 0;
  int inputIndex = 0;
  int classIndex = 0;

  auto ctpClasses = mCTPconfig.getCTPClasses();
  auto ctpInputs = mCTPconfig.getCTPInputs();

  std::vector<int> nonExistingInputs = { 11, 12, 19, 20, 23, 24, 30, 31, 32, 33, 34, 35, 36, 45, 46, 47 }; // histograms for these inputs are not defined in the config

  const int numberOfInputs = 48 - nonExistingInputs.size();
  ILOG(Info, Support) << "numberOfInputs.  " << numberOfInputs << ENDM;

  std::vector<int> trendedInputIndex;
  std::vector<int> trendedClassIndex;
  for (int i = 0; i < ctpInputs.size(); i++) {
    auto input = ctpInputs[i];
    if (std::find(nonExistingInputs.begin(), nonExistingInputs.end(), input.getIndex()) != std::end(nonExistingInputs)) {
      ILOG(Info, Support) << "Input " << input.getIndex() << " should not exist!!" << ENDM;
      continue;
    }
    if (trendedInputIndex.size() == 0)
      trendedInputIndex.push_back(input.getIndex());
    else if (std::find(trendedInputIndex.begin(), trendedInputIndex.end(), input.getIndex()) != std::end(trendedInputIndex))
      continue;
    else
      trendedInputIndex.push_back(input.getIndex());
  }

  for (int i = 0; i < ctpClasses.size(); i++) {
    auto ctpClass = ctpClasses[i];
    if (i == 0)
      trendedClassIndex.push_back(ctpClass.getIndex());
    else if (std::find(trendedClassIndex.begin(), trendedClassIndex.end(), ctpClass.getIndex()) != std::end(trendedClassIndex))
      continue;
    else
      trendedClassIndex.push_back(ctpClass.getIndex());
  }

  ILOG(Info, Support) << "Generating " << trendedClassIndex.size() + trendedInputIndex.size() << " plots." << ENDM;

  for (const auto& plot : mConfig.plots) {

    if (index < numberOfInputs && index != trendedInputIndex[inputIndex] - 1) {
      index++;
      continue;
    } else if (index >= numberOfInputs && index - numberOfInputs != trendedClassIndex[classIndex]) {
      index++;
      continue;
    } else {
      // Before we generate any new plots, we have to delete existing under the same names.
      // It seems that ROOT cannot handle an existence of two canvases with a common name in the same process.
      if (mPlots.count(plot.name)) {
        getObjectsManager()->stopPublishing(plot.name);
        delete mPlots[plot.name];
      }

      auto* c = new TCanvas();
      ILOG(Info, Support) << plot.varexp << " " << plot.selection << " " << plot.option << ENDM;
      mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), plot.option.c_str());

      c->SetName(plot.name.c_str());
      c->SetTitle(plot.title.c_str());

      if (auto histo = dynamic_cast<TH1*>(c->GetPrimitive("htemp"))) {
        if (index < numberOfInputs)
          histo->SetTitle(ctpInputs[inputIndex].name.c_str());
        else
          histo->SetTitle(ctpClasses[classIndex].name.c_str());
        histo->GetYaxis()->SetTitle("rate [Hz]");
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
      getObjectsManager()->startPublishing(c);

      if (index < numberOfInputs)
        inputIndex++;
      else
        classIndex++;
      index++;
    }
  }
}
