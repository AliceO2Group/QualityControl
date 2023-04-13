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
/// \file    TrendingTaskITSFhr.cxx
/// \author  Ivan Ravasenga on the structure from Piotr Konopka
///

#include "ITS/TrendingTaskITSFhr.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "ITS/TH2XlineReductor.h"
#include <TCanvas.h>
#include <TH1.h>
#include <TMultiGraph.h>
#include <TDatime.h>
#include <map>
#include <string>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;
using namespace o2::quality_control_modules::its;

void TrendingTaskITSFhr::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigITS(getID(), config);
}

void TrendingTaskITSFhr::initialize(Trigger, framework::ServiceRegistryRef)
{
  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>(); // todo: retrieve last TTree, so we
                                      // continue trending. maybe do it
                                      // optionally?
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  // mTrend->Branch("meta", &mMetaData, "runNumber/I");
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
void TrendingTaskITSFhr::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  trendValues(t, qcdb);
  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSFhr::finalize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSFhr::storeTrend(repository::DatabaseInterface& qcdb)
{
  ILOG(Debug, Devel) << "Storing the trend, entries: " << mTrend->GetEntries() << ENDM;

  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(), "o2::quality_control_modules::its::TrendingTaskITSFhr",
                                                  mConfig.detectorName, mMetaData.runNumber);
  mo->setIsOwner(false);
  qcdb.storeMO(mo);
}

void TrendingTaskITSFhr::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
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

void TrendingTaskITSFhr::storePlots(repository::DatabaseInterface& qcdb)
{
  //
  // Create and save trends for each stave
  //
  int countplots = 0;
  int ilay = 0;
  double ymin[NTRENDSFHR] = { 1e-15, 1e-1, -.5, 1e-9 };
  double ymaxIB[NTRENDSFHR] = { 1e-3, 1e-5, 9.5, 1 };
  double ymaxOB[NTRENDSFHR] = { 1e-3, 1e-5, 30.5, 1 };

  // There was previously loop on plots for each stave
  // Removed to reduce number of saved histograms
  //
  //  Create canvas with multiple trends - average threshold - 1 canvas per layer
  //
  ilay = 0;
  countplots = 0;
  TCanvas* c[NLAYERS * NTRENDSFHR];
  TMultiGraph* gTrendsAll[NLAYERS * NTRENDSFHR];
  TLegend* legstaves[NLAYERS];
  for (int idx = 0; idx < NLAYERS * NTRENDSFHR; idx++) { // define canvases
    if (idx < 13) {
      c[idx] = new TCanvas(
        Form("fhr_%s_trends_L%d", trendnamesIB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR),
        Form("fhr_%s_trends_L%d", trendnamesIB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR));

      gTrendsAll[idx] = new TMultiGraph(
        Form("fhr_%s_trends_L%d_multigraph", trendnamesIB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR),
        Form("fhr_%s_trends_L%d_multigraph", trendnamesIB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR));
    } else {
      c[idx] = new TCanvas(
        Form("fhr_%s_trends_L%d", trendnamesOB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR),
        Form("fhr_%s_trends_L%d", trendnamesOB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR));

      gTrendsAll[idx] = new TMultiGraph(
        Form("fhr_%s_trends_L%d_multigraph", trendnamesOB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR),
        Form("fhr_%s_trends_L%d_multigraph", trendnamesOB[idx % NTRENDSFHR].c_str(),
             idx / NTRENDSFHR));
    }
  }

  for (int ilay = 0; ilay < NLAYERS; ilay++) { // define legends
    legstaves[ilay] = new TLegend(0.91, 0.1, 0.98, 0.9);
    legstaves[ilay]->SetName(Form("legstaves_L%d", ilay));
    SetLegendStyle(legstaves[ilay]);
    PrepareLegend(legstaves[ilay], ilay);
  }
  ilay = 0;

  for (const auto& plot : mConfig.plots) {

    int colidx = countplots % 7;
    int mkridx = countplots / 7;
    int index = 0;
    if (plot.name.find("occ") != std::string::npos)
      index = 3;
    else if (plot.name.find("chips") != std::string::npos || plot.name.find("lanes") != std::string::npos)
      index = 2;
    else if (plot.name.find("stddev") != std::string::npos)
      index = 1;
    else
      index = 0;

    bool isrun = plot.varexp.find("ntreeentries") != std::string::npos ? true : false; // vs run or vs time
    long int n = mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), "goff");
    // post processing plot
    ILOG(Debug, Devel) << " Drawing " << plot.name << ENDM;
    TGraph* g = new TGraph(n, mTrend->GetV2(), mTrend->GetV1());
    SetGraphStyle(g, col[colidx], mkr[mkridx]);
    gTrendsAll[ilay * NTRENDSFHR + index]->Add((TGraph*)g->Clone());
    delete g;

    if (plot.name.find("occ") != std::string::npos)
      countplots++;

    if (countplots > nStaves[ilay] - 1) {
      for (int id = 0; id < 4; id++) {
        c[ilay * NTRENDSFHR + id]->cd();
        c[ilay * NTRENDSFHR + id]->SetTickx();
        c[ilay * NTRENDSFHR + id]->SetTicky();
        if (id != 2)
          c[ilay * NTRENDSFHR + id]->SetLogy();

        int npoints = (int)runlist.size();
        TH1F* hfake = new TH1F("hfake", "hfake", npoints, 0.5, (double)npoints + 0.5);
        if (ilay < 3) {
          SetGraphNameAndAxes(hfake,
                              Form("L%d - %s trends", ilay, trendtitlesIB[id].c_str()),
                              isrun ? "run" : "time", ytitlesIB[id], ymin[id], ymaxIB[id], runlist);
        } else {
          SetGraphNameAndAxes(hfake,
                              Form("L%d - %s trends", ilay, trendtitlesOB[id].c_str()),
                              isrun ? "run" : "time", ytitlesOB[id], ymin[id], ymaxOB[id], runlist);
        }

        hfake->Draw();
        gTrendsAll[ilay * NTRENDSFHR + id]->Draw();
        legstaves[ilay]->Draw("same");

        ILOG(Debug, Devel) << " Saving canvas for layer " << ilay << " to CCDB "
                           << ENDM;
        auto mo = std::make_shared<MonitorObject>(c[ilay * NTRENDSFHR + id], mConfig.taskName, "o2::quality_control_modules::its::TrendingTaskITSFhr",
                                                  mConfig.detectorName, mMetaData.runNumber);
        mo->setIsOwner(false);
        qcdb.storeMO(mo);
        delete c[ilay * NTRENDSFHR + id];
        delete gTrendsAll[ilay * NTRENDSFHR + id];
        delete hfake;
      }
      countplots = 0;
      ilay++;
    }
  } // end loop on plots

  for (int idx = 0; idx < NLAYERS; idx++) {
    delete legstaves[idx];
  }
}

void TrendingTaskITSFhr::SetLegendStyle(TLegend* leg)
{
  leg->SetTextFont(42);
  leg->SetLineColor(kWhite);
  leg->SetFillColor(0);
}

void TrendingTaskITSFhr::SetGraphStyle(TGraph* g, int col, int mkr)
{
  g->SetLineColor(col);
  g->SetMarkerStyle(mkr);
  g->SetMarkerColor(col);
}

void TrendingTaskITSFhr::SetGraphNameAndAxes(TH1* g,
                                             std::string title, std::string xtitle,
                                             std::string ytitle, double ymin,
                                             double ymax, std::vector<std::string> runlist)
{
  g->SetStats(0);
  g->SetTitle(title.c_str());
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
    for (int ipoint = 0; ipoint < runlist.size(); ipoint++) {
      g->GetXaxis()->SetBinLabel(g->GetXaxis()->FindBin(ipoint + 1.), runlist[ipoint].c_str());
    }
  }
}

void TrendingTaskITSFhr::PrepareLegend(TLegend* leg, int layer)
{
  for (int istv = 0; istv < nStaves[layer]; istv++) {
    int colidx = istv % 7;
    int mkridx = istv / 7;
    TGraph* gr = new TGraph(); // dummy histo
    SetGraphStyle(gr, col[colidx], mkr[mkridx]);
    leg->AddEntry(gr, Form("%02d", istv), "pl");
  }
}
