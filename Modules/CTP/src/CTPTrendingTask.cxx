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
#include <QualityControl/RootClassFactory.h>
#include <QualityControl/QcInfoLogger.h>
#include <QualityControl/ActivityHelpers.h>
#include <CCDB/BasicCCDBManager.h>
#include <DataFormatsCTP/Configuration.h>
// #include "DataFormatsCTP/RunManager.h"

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

/// this function initialize all necessary parameters
void CTPTrendingTask::initCTP(Trigger& t)
{
  std::string run = std::to_string(t.activity.mId);
  std::string CCDBHost;
  try {
    CCDBHost = mCustomParameters.at("ccdbName", "default");
  } catch (const std::exception& e) {
    CCDBHost = "https://alice-ccdb.cern.ch";
  }

  /// the reading of the ccdb from trending was already discussed and is related with the fact that CTPconfing may not be ready at the QC starts
  // o2::ctp::CTPRunManager::setCCDBHost(CCDBHost);
  bool ok;
  // o2::ctp::CTPConfiguration CTPconfig = o2::ctp::CTPRunManager::getConfigFromCCDB(t.timestamp, run, ok);
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setURL(CCDBHost);
  map<string, string> metadata; // can be empty
  metadata["runNumber"] = run;
  auto ctpconfigdb = mgr.getSpecific<o2::ctp::CTPConfiguration>(o2::ctp::CCDBPathCTPConfig, t.timestamp, metadata);
  if (ctpconfigdb == nullptr) {
    LOG(info) << "CTP config not in database, timestamp:" << t.timestamp;
    ok = 0;
  } else {
    // ctpconfigdb->printStream(std::cout);
    LOG(info) << "CTP config found. Run:" << run;
    ok = 1;
  }
  if (!ok) {
    ILOG(Warning, Support) << "CTP Config not found for run:" << run << " timesamp " << t.timestamp << ENDM;
    return;
  } else {
    mCTPconfigFound = true;
  }

  try {
    mClassNames[0] = std::stof(mCustomParameters.at("minBias1Class", "default"));
  } catch (const std::exception& e) {
    mClassNames[0] = mClassNamesDefault[0];
  }

  try {
    mClassNames[1] = std::stof(mCustomParameters.at("minBias2Class", "default"));
  } catch (const std::exception& e) {
    mClassNames[1] = mClassNamesDefault[1];
  }

  try {
    mClassNames[2] = std::stof(mCustomParameters.at("minBisDMCclass", "default"));
  } catch (const std::exception& e) {
    mClassNames[2] = mClassNamesDefault[2];
  }

  try {
    mClassNames[3] = std::stof(mCustomParameters.at("minBiasEMCclass", "default"));
  } catch (const std::exception& e) {
    mClassNames[3] = mClassNamesDefault[3];
  }

  try {
    mClassNames[4] = std::stof(mCustomParameters.at("minBiasPHOclass", "default"));
  } catch (const std::exception& e) {
    mClassNames[4] = mClassNamesDefault[4];
  }

  try {
    mInputNames[0] = std::stof(mCustomParameters.at("minBias1Input", "default"));
  } catch (const std::exception& e) {
    mInputNames[0] = mInputNamesDefault[0];
  }

  try {
    mInputNames[1] = std::stof(mCustomParameters.at("minBias2Input", "default"));
  } catch (const std::exception& e) {
    mInputNames[1] = mInputNamesDefault[1];
  }

  try {
    mInputNames[2] = std::stof(mCustomParameters.at("minBisDMCInput", "default"));
  } catch (const std::exception& e) {
    mInputNames[2] = mInputNamesDefault[2];
  }

  try {
    mInputNames[3] = std::stof(mCustomParameters.at("minBiasEMCInput", "default"));
  } catch (const std::exception& e) {
    mInputNames[3] = mInputNamesDefault[3];
  }

  try {
    mInputNames[4] = std::stof(mCustomParameters.at("minBiasPHOInput", "default"));
  } catch (const std::exception& e) {
    mInputNames[4] = mInputNamesDefault[4];
  }

  // get the indices of the classes we want to trend
  // std::vector<ctp::CTPClass> ctpcls = CTPconfig.getCTPClasses();
  // std::vector<int> clslist = CTPconfig.getTriggerClassList();
  std::vector<ctp::CTPClass> ctpcls = ctpconfigdb->getCTPClasses();
  std::vector<int> clslist = ctpconfigdb->getTriggerClassList();
  for (size_t i = 0; i < clslist.size(); i++) {
    for (size_t j = 0; j < mNumberOfClasses; j++) {
      if (ctpcls[i].name.find(mClassNames[j]) != std::string::npos) {
        mClassIndex[j] = ctpcls[i].descriptorIndex + 1;
        break;
      }
    }
  }

  for (size_t i = 0; i < sizeof(ctpinputs) / sizeof(std::string); i++) {
    for (size_t j = 0; j < mNumberOfInputs; j++) {
      if (ctpinputs[i].find(mInputNames[j]) != std::string::npos) {
        mInputIndex[j] = i + 1;
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
    reductor->SetClassIndexes(mClassIndex[0], mClassIndex[1], mClassIndex[2], mClassIndex[3], mClassIndex[4]);
    reductor->SetInputIndexes(mInputIndex[0], mInputIndex[1], mInputIndex[2], mInputIndex[3], mInputIndex[4]);
    mTrend->Branch(sourceName.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
  }

  getObjectsManager()->startPublishing(mTrend.get());
}
void CTPTrendingTask::initialize(Trigger t, framework::ServiceRegistryRef services)
{
}

void CTPTrendingTask::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  if (!mCTPconfigFound) {
    initCTP(t);
  }
  if (!mCTPconfigFound) {
    return;
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

    if (index < 5 && mInputIndex[index] == 49) { // if the input index == 49, this input won't be trended
      ILOG(Info, Support) << "Input " << mInputNames[index] << " is not trended." << ENDM;
      index++;
      continue;
    }

    if (index > 4 && index < 10 && mClassIndex[index - 5] == 65) { // if the class index == 65, this ctp class is not defined in the CTPconfig, so it won't be trended
      ILOG(Info, Support) << "Class " << mClassNames[index - 5] << " is not trended." << ENDM;
      index++;
      continue;
    }

    if (index > 9 && index < 14 && (mInputIndex[index - 9] == 49 || mInputIndex[0] == 49)) { // if the input index == 49, this ctp input won't be trended
      ILOG(Info, Support) << "Input ratio " << mInputNames[index - 13] << " / " << mInputNames[0] << " is not trended." << ENDM;
      index++;
      continue;
    }

    if (index > 13 && (mClassIndex[index - 13] == 65 || mClassIndex[0] == 65)) { // if the class index == 65, this ctp class is not defined in the CTPconfig, so it won't be trended
      ILOG(Info, Support) << "Class ratio " << mClassNames[index - 13] << " / " << mClassNames[0] << " is not trended." << ENDM;
      index++;
      continue;
    }

    auto* c = new TCanvas();
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
