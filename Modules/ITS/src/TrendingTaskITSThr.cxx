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
/// \file    TrendingTaskITSThr.cxx
/// \author  Ivan Ravasenga on the structure from Piotr Konopka
///

#include "ITS/TrendingTaskITSThr.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include <TCanvas.h>
#include <TH1.h>
#include <TMultiGraph.h>
#include <TDatime.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;

void TrendingTaskITSThr::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigITS(getID(), config);
}

void TrendingTaskITSThr::initialize(Trigger, framework::ServiceRegistryRef)
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
void TrendingTaskITSThr::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSThr::finalize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSThr::storeTrend(repository::DatabaseInterface& qcdb)
{
  ILOG(Debug, Devel) << "Storing the trend, entries: " << mTrend->GetEntries() << ENDM;

  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(),
                                                  mConfig.className,
                                                  mConfig.detectorName, mMetaData.runNumber);
  mo->setIsOwner(false);
  qcdb.storeMO(mo);
}

void TrendingTaskITSThr::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  // We use current date and time. This for planned processing (not history). We
  // still might need to use the objects
  // timestamps in the end, but this would become ambiguous if there is more
  // than one data source.
  mTime = TDatime().Convert();
  mMetaData.runNumber = t.activity.mId;
  int count = 0;

  for (auto& dataSource : mConfig.dataSources) {

    // todo: make it agnostic to MOs, QOs or other objects. Let the reductor
    // cast to whatever it needs.
    if (dataSource.type == "repository") {
      // auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name);
      auto mo = qcdb.retrieveMO(dataSource.path, "", t.timestamp, t.activity);
      if (mo == nullptr)
        continue;
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
void TrendingTaskITSThr::storePlots(repository::DatabaseInterface& qcdb)
{
  //
  // Create canvas with multiple trends - average threshold - 1 canvas per layer
  //
  int ilay = 0;
  int countplots = 0;
  TCanvas* c[NLAYERS * NTRENDSTHR];
  TMultiGraph* gTrends_all[NLAYERS * NTRENDSTHR];

  TLegend* legstaves[NLAYERS];
  for (int idx = 0; idx < NLAYERS * NTRENDSTHR; idx++) { // define canvases
    c[idx] = new TCanvas(
      Form("threshold_%s_trends_L%d", trendnames[idx % NTRENDSTHR].c_str(),
           idx / NTRENDSTHR),
      Form("threshold_%s_trends_L%d", trendnames[idx % NTRENDSTHR].c_str(),
           idx / NTRENDSTHR));

    gTrends_all[idx] = new TMultiGraph(
      Form("threshold_%s_trends_L%d", trendnames[idx % NTRENDSTHR].c_str(),
           idx / NTRENDSTHR),
      Form("threshold_%s_trends_L%d", trendnames[idx % NTRENDSTHR].c_str(),
           idx / NTRENDSTHR));
  }

  for (int ilay = 0; ilay < NLAYERS; ilay++) { // define legends
    legstaves[ilay] = new TLegend(0.91, 0.1, 0.98, 0.9);
    legstaves[ilay]->SetName(Form("legstaves_L%d", ilay));
    SetLegendStyle(legstaves[ilay]);
    PrepareLegend(legstaves[ilay], ilay);
  }
  ilay = 0;
  for (const auto& plot : mConfig.plots) {
    int colidx = countplots > 41 ? countplots - 42 : countplots > 34 ? countplots - 35
                                                   : countplots > 27 ? countplots - 28
                                                   : countplots > 20 ? countplots - 21
                                                   : countplots > 13 ? countplots - 14
                                                   : countplots > 6  ? countplots - 7
                                                                     : countplots;
    int mkridx = countplots > 41 ? 6 : countplots > 34 ? 5
                                     : countplots > 27 ? 4
                                     : countplots > 20 ? 3
                                     : countplots > 13 ? 2
                                     : countplots > 6  ? 1
                                                       : 0;
    int add = (plot.name.find("rms") != std::string::npos)
                ? 1
              : plot.name.find("Active") != std::string::npos ? 2
                                                              : 0;
    bool isrun = 1; // time no longer needed
    long int n = mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), "goff");
    TGraph* g = new TGraph(n, mTrend->GetV2(), mTrend->GetV1());
    SetGraphStyle(g, col[colidx], mkr[mkridx]);
    gTrends_all[ilay * NTRENDSTHR + add]->Add((TGraph*)g->Clone());
    delete g;
    if (plot.name.find("Active") != std::string::npos) {
      countplots++;
    }

    if (countplots > nStaves[ilay] - 1) {
      for (int id = 0; id < NTRENDSTHR; id++) {
        c[ilay * NTRENDSTHR + id]->cd();
        c[ilay * NTRENDSTHR + id]->SetTickx();
        c[ilay * NTRENDSTHR + id]->SetTicky();
        if (id == 2)
          c[ilay * NTRENDSTHR + id]->SetLogy();
        double ymin = 0.;
        double ymax = id == 1
                        ? 20.
                      : id == 2 ? 100
                                : 250.;
        int npoints = (int)runlist.size();
        TH1F* hfake = new TH1F("hfake", "hfake", npoints, 0.5, (double)npoints + 0.5);
        hfake->SetStats(0);
        SetGraphNameAndAxes(hfake, "hfake", Form("L%d - %s trends", ilay, trendtitles[id].c_str()), isrun ? "run" : "time", ytitles[id], ymin, ymax, runlist);
        hfake->Draw();
        gTrends_all[ilay * NTRENDSTHR + id]->Draw();
        legstaves[ilay]->Draw();
        ILOG(Debug, Devel) << " Saving canvas for layer " << ilay << " to CCDB "
                           << ENDM;
        auto mo = std::make_shared<MonitorObject>(c[ilay * NTRENDSTHR + id], mConfig.taskName, "o2::quality_control_modules::its::TrendingTaskITSThr",
                                                  mConfig.detectorName);
        mo->setIsOwner(false);
        qcdb.storeMO(mo);

        delete hfake;
        delete gTrends_all[ilay * NTRENDSTHR + id];
        delete c[ilay * NTRENDSTHR + id];
      }

      countplots = 0;
      ilay++;
    }
  } // end loop on plots

  for (int ilayer = 0; ilayer < NLAYERS; ilayer++) {
    delete legstaves[ilayer];
  }
}

void TrendingTaskITSThr::SetLegendStyle(TLegend* leg)
{
  leg->SetTextFont(42);
  leg->SetLineColor(kWhite);
  leg->SetFillColor(0);
}

void TrendingTaskITSThr::SetGraphStyle(TGraph* g, int col, int mkr)
{
  g->SetLineColor(col);
  g->SetMarkerStyle(mkr);
  g->SetMarkerColor(col);
}

void TrendingTaskITSThr::SetGraphNameAndAxes(TH1* g, std::string name,
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
    for (int ipoint = 0; ipoint < (int)runlist.size(); ipoint++) {
      g->GetXaxis()->SetBinLabel(g->GetXaxis()->FindBin(ipoint + 1.), runlist[ipoint].c_str());
    }
  }
}

void TrendingTaskITSThr::PrepareLegend(TLegend* leg, int layer)
{
  for (int istv = 0; istv < nStaves[layer]; istv++) {
    int colidx = istv > 41 ? istv - 42 : istv > 34 ? istv - 35
                                       : istv > 27 ? istv - 28
                                       : istv > 20 ? istv - 21
                                       : istv > 13 ? istv - 14
                                       : istv > 6  ? istv - 7
                                                   : istv;
    int mkridx = istv > 41 ? 6 : istv > 34 ? 5
                               : istv > 27 ? 4
                               : istv > 20 ? 3
                               : istv > 13 ? 2
                               : istv > 6  ? 1
                                           : 0;
    TGraph* gr = new TGraph(); // dummy histo
    SetGraphStyle(gr, col[colidx], mkr[mkridx]);
    leg->AddEntry(gr, Form("%02d", istv), "pl");
  }
}
