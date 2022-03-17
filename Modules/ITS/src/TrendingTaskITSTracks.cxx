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
/// \file    TrendingTaskITSTracks.cxx
/// \author  Ivan Ravasenga on the structure from Piotr Konopka
///

#include "ITS/TrendingTaskITSTracks.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Reductor.h"
#include <TCanvas.h>
#include <TH1.h>
#include <TDatime.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

void TrendingTaskITSTracks::configure(std::string name,
                                      const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigITS(name, config);
}

void TrendingTaskITSTracks::initialize(Trigger, framework::ServiceRegistry&)
{
  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>(); // todo: retrieve last TTree, so we
                                      // continue trending. maybe do it
                                      // optionally?
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("ntreeentries", &ntreeentries);
  mTrend->Branch("time", &mTime);

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<Reductor> reductor(root_class_factory::create<Reductor>(
      source.moduleName, source.reductorName));
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(),
                   reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TrendingTaskITSTracks::update(Trigger t, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSTracks::finalize(Trigger t, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSTracks::storeTrend(repository::DatabaseInterface& qcdb)
{
  ILOG(Info, Support) << "Storing the trend, entries: " << mTrend->GetEntries() << ENDM;

  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(),
                                                  mConfig.className,
                                                  mConfig.detectorName);
  mo->setIsOwner(false);
  qcdb.storeMO(mo);
}

void TrendingTaskITSTracks::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  // We use current date and time. This for planned processing (not history). We
  // still might need to use the objects
  // timestamps in the end, but this would become ambiguous if there is more
  // than one data source.
  mTime = TDatime().Convert();
  // todo get run number when it is available. consider putting it inside
  // monitor object's metadata (this might be not
  //  enough if we trend across runs).
  mMetaData.runNumber = 0;
  int count = 0;

  for (auto& dataSource : mConfig.dataSources) {
    //std::cout<<"TrendingTaskITSTracks dataSource type "<<dataSource.name<<" "<<dataSource.type<<std::endl;
    // todo: make it agnostic to MOs, QOs or other objects. Let the reductor
    // cast to whatever it needs.
    if (dataSource.type == "repository") {
      // auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name);
      auto mo = qcdb.retrieveMO(dataSource.path, "", t.timestamp, t.activity);
      if (!count) {
        std::map<std::string, std::string> entryMetadata = mo->getMetadataMap(); //full list of metadata as a map
        mMetaData.runNumber = std::stoi(entryMetadata["RunNumber"]);             //get and set run number
        ntreeentries = (Int_t)mTrend->GetEntries() + 1;
        runlist.push_back(std::to_string(mMetaData.runNumber));
      }
      TObject* obj = mo ? mo->getObject() : nullptr;
      if (obj) {
        mReductors[dataSource.name]->update(obj);
      }
    } else if (dataSource.type == "repository-quality") {
      auto qo = qcdb.retrieveQO(dataSource.path + "/" + dataSource.name);
      if (qo) {
        mReductors[dataSource.name]->update(qo.get());
      }
    } else {
      ILOGE << "Unknown type of data source '" << dataSource.type << "'.";
    }
    count++;
  }
  mTrend->Fill();
}

void TrendingTaskITSTracks::storePlots(repository::DatabaseInterface& qcdb)
{
  ILOG(Info, Support) << "Generating and storing " << mConfig.plots.size() << " plots."
                      << ENDM;
  //
  // Create and save trends for each stave
  //
  int countplots = 0;
  int ilay = 0;
  for (const auto& plot : mConfig.plots) {

    int add = 0;
    double ymin = -10.;
    double ymax = +10.;
    if (plot.name.find("mean") != std::string::npos) {
      add = 0;
      ymin = 0.;
      ymax = 15.;
    } else if (plot.name.find("stddev") != std::string::npos) {
      add = 1;
      ymin = 0.;
      ymax = 5.;
    }

    bool isrun = plot.varexp.find("ntreeentries") != std::string::npos ? true : false; // vs run or vs time
    long int n = mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(),
                              "goff");

    double* x = mTrend->GetV2();
    double* y = mTrend->GetV1();

    // post processing plot
    TGraph* g = new TGraph(n, x, y);
    SetGraphStyle(g, col[add], mkr[add]);

    SetGraphNameAndAxes(g, plot.name, plot.title, isrun ? "run" : "time", ytitles[add], ymin,
                        ymax, runlist);
    ILOG(Info, Support) << " Saving " << plot.name << " to CCDB " << ENDM;
    auto mo = std::make_shared<MonitorObject>(g, mConfig.taskName, "o2::quality_control_modules::its::TrendingTaskITSTracks",
                                              mConfig.detectorName);
    mo->setIsOwner(false);
    qcdb.storeMO(mo);

    // It should delete everything inside. Confirmed by trying to delete histo
    // after and getting a segfault.
    delete g;
  } // end loop on plots
}

void TrendingTaskITSTracks::SetLegendStyle(TLegend* leg)
{
  leg->SetTextFont(42);
  leg->SetLineColor(kWhite);
  leg->SetFillColor(0);
}

void TrendingTaskITSTracks::SetGraphStyle(TGraph* g, int col, int mkr)
{
  g->SetLineColor(col);
  g->SetMarkerStyle(mkr);
  g->SetMarkerColor(col);
}

void TrendingTaskITSTracks::SetGraphNameAndAxes(TGraph* g, std::string name,
                                                std::string title, std::string xtitle,
                                                std::string ytitle, double ymin,
                                                double ymax, std::vector<std::string> runlist)
{
  g->SetTitle(title.c_str());
  g->SetName(name.c_str());

  g->GetXaxis()->SetTitle(xtitle.c_str());
  g->GetYaxis()->SetTitle(ytitle.c_str());
  g->GetYaxis()->SetRangeUser(ymin, ymax);

  if (xtitle.find("time") != std::string::npos) {
    g->GetXaxis()->SetTimeDisplay(1);
    // It deals with highly congested dates labels
    g->GetXaxis()->SetNdivisions(505);
    // Without this it would show dates in order of 2044-12-18 on the day of
    // 2019-12-19.
    g->GetXaxis()->SetTimeOffset(0.0);
    g->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
  }
  if (xtitle.find("run") != std::string::npos) {
    g->GetXaxis()->SetNdivisions(505); // It deals with highly congested dates labels
    for (int ipoint = 0; ipoint < g->GetN(); ipoint++) {
      g->GetXaxis()->SetBinLabel(g->GetXaxis()->FindBin(ipoint + 1.), runlist[ipoint].c_str());
      g->GetXaxis()->LabelsOption("v");
    }
  }
}

void TrendingTaskITSTracks::PrepareLegend(TLegend* leg, int layer)
{
}
