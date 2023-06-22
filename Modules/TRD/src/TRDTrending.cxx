
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
/// \file TRDTrending.cxx
/// \author      based on Piotr Konopka
///

#include "TRD/TRDTrending.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/ActivityHelpers.h"
#include <TDatime.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TGraphErrors.h>
#include <TPoint.h>
#include <map>
#include <string>
#include <memory>
#include <vector>
using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace o2::quality_control::postprocessing;

void TRDTrending::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigTRD(getID(), config);
}

void TRDTrending::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);

  mTrend->Branch("ntreeentries", &ntreeentries);
  mTrend->Branch("time", &mTime);

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<Reductor> reductor(root_class_factory::create<Reductor>(source.moduleName, source.reductorName));

    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TRDTrending::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  trendValues(t, qcdb);
  generatePlots(qcdb);
}

void TRDTrending::finalize(Trigger, framework::ServiceRegistryRef services)
{

  auto& qcdb = services.get<repository::DatabaseInterface>();
  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(), "o2::quality_control_modules::trd::TRDTrending", mConfig.detectorName);
  mo->setIsOwner(false);
  qcdb.storeMO(mo);
  generatePlots(qcdb);
}

void TRDTrending::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  mTime = TDatime().Convert(); // ROOT expects seconds since epoch.
  mMetaData.runNumber = t.activity.mId;
  int count = 0;

  for (auto& dataSource : mConfig.dataSources) {

    // todo: make it agnostic to MOs, QOs or other objects. Let the reductor cast to whatever it needs.
    if (dataSource.type == "repository") {
      auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp);
      if (mo == nullptr) {
        ILOG(Warning, Devel) << "Could not retrieve MO '" << dataSource.name << "' from QCDB, skipping this data source" << ENDM;
        continue;
      }

      if (!count) {
        std::map<std::string, std::string> entryMetadata = mo->getMetadataMap();  // full list of metadata as a map
        mMetaData.runNumber = std::stoi(entryMetadata[metadata_keys::runNumber]); // get and set run number
        ntreeentries = (Int_t)mTrend->GetEntries() + 1;
        runlist.push_back(std::to_string(mMetaData.runNumber));
      }
      TObject* obj = mo ? mo->getObject() : nullptr;
      if (obj) {
        mReductors[dataSource.name]->update(obj);
      }
    } else {
      ILOG(Error, Support) << "Unknown type of data source '" << dataSource.type << "'." << ENDM;
    }
    count++;
  }

  mTrend->Fill();
}

void TRDTrending::generatePlots(repository::DatabaseInterface& qcdb)
{
  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, won't generate any plots." << ENDM;
    return;
  }

  ILOG(Info, Support) << "Generating " << mConfig.plots.size() << " plots." << ENDM;

  std::vector<std::unique_ptr<TCanvas>> mCanvasTRD;
  int mPlot = 0;
  for (const auto& plot : mConfig.plots) {

    mCanvasTRD.push_back(std::make_unique<TCanvas>(plot.name.c_str(), plot.title.c_str()));
    mCanvasTRD[mPlot].get()->cd();

    mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), plot.option.c_str());

    if (auto histo = dynamic_cast<TH1*>(mCanvasTRD[mPlot].get()->GetPrimitive("htemp"))) {

      histo->SetTitle(plot.title.c_str());
      mCanvasTRD[mPlot]->Update();
      if (plot.varexp.find(":time") != std::string::npos) {
        histo->GetXaxis()->SetTimeDisplay(1);

        // It deals with highly congested dates labels
        histo->GetXaxis()->SetNdivisions(505);
        // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
        histo->GetXaxis()->SetTimeOffset(0.0);
        histo->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");

      }

      else if (plot.varexp.find(":runNumber") != std::string::npos) {
        histo->GetXaxis()->SetNdivisions(505);

        for (int ir = 0; ir < (int)runlist.size(); ir++)
          histo->GetXaxis()->SetBinLabel(ir + 1, runlist[ir].c_str());
      }

      histo->BufferEmpty();

    } else {
      ILOG(Error, Devel) << "Could not get the processing histogram of the plot '" << plot.name << "'." << ENDM;
    }

    mPlots[plot.name] = mCanvasTRD[mPlot].get();

    auto mo_trd = std::make_shared<MonitorObject>(mCanvasTRD[mPlot].get(), mConfig.taskName, "o2::quality_control_modules::trd::TRDTrending", mConfig.detectorName);

    mo_trd->setIsOwner(false);
    qcdb.storeMO(mo_trd);

    ++mPlot;
  }
}
