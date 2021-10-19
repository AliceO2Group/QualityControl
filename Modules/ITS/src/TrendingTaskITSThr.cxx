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
#include <TCanvas.h>
#include <TH1.h>
#include <TDatime.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

void TrendingTaskITSThr::configure(std::string name,
                                   const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigITS(name, config);
}

void TrendingTaskITSThr::initialize(Trigger, framework::ServiceRegistry&)
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
void TrendingTaskITSThr::update(Trigger, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(qcdb);

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSThr::finalize(Trigger, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSThr::storeTrend(repository::DatabaseInterface& qcdb)
{
  ILOG(Info, Support) << "Storing the trend, entries: " << mTrend->GetEntries() << ENDM;

  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(),
                                                  mConfig.className,
                                                  mConfig.detectorName);
  mo->setIsOwner(false);
  qcdb.storeMO(mo);
}

void TrendingTaskITSThr::trendValues(repository::DatabaseInterface& qcdb)
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

    // todo: make it agnostic to MOs, QOs or other objects. Let the reductor
    // cast to whatever it needs.
    if (dataSource.type == "repository") {
      // auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name);
      auto mo = qcdb.retrieveMO(dataSource.path, "");
      if (!count) {
        std::map<std::string, std::string> entryMetadata = mo->getMetadataMap(); //full list of metadata as a map
        mMetaData.runNumber = std::stoi(entryMetadata["Run"]);                   //get and set run number
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
  ILOG(Info, Support) << "Generating and storing " << mConfig.plots.size() << " plots."
                      << ENDM;
  //
  // Create and save trends for each stave
  //
  int countplots = 0;
  int ilay = 0;
  for (const auto& plot : mConfig.plots) {
    if (countplots > nStaves[ilay] - 1) {
      countplots = 0;
      ilay++;
    }
    int colidx = countplots > 13 ? countplots - 14
                                 : countplots > 6 ? countplots - 7 : countplots;
    int mkridx = countplots > 13 ? 2 : countplots > 6 ? 1 : 0;
    int add = (plot.name.find("rms") != std::string::npos)
                ? 1
                : plot.name.find("dead") != std::string::npos ? 2 : 0;

    bool isrun = plot.varexp.find("ntreeentries") != std::string::npos ? true : false; // vs run or vs time
    long int n = mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(),
                              "goff"); // plot.option.c_str());
    // post processing plot
    TGraph* g = new TGraph(n, mTrend->GetV2(), mTrend->GetV1());
    SetGraphStyle(g, col[colidx], mkr[mkridx]);
    double ymin = plot.name.find("rms") != std::string::npos
                    ? 0.
                    : plot.name.find("dead") != std::string::npos ? 1e-1 : 7.;
    double ymax = plot.name.find("rms") != std::string::npos
                    ? 5.
                    : plot.name.find("dead") != std::string::npos ? 5e3 : 14.;
    SetGraphNameAndAxes(g, plot.name, plot.title, isrun ? "run" : "time", ytitles[add], ymin,
                        ymax, runlist);
    ILOG(Info, Support) << " Saving " << plot.name << " to CCDB " << ENDM;
    auto mo = std::make_shared<MonitorObject>(g, mConfig.taskName, "o2::quality_control_modules::its::TrendingTaskITSThr",
                                              mConfig.detectorName);
    mo->setIsOwner(false);
    qcdb.storeMO(mo);

    // It should delete everything inside. Confirmed by trying to delete histo
    // after and getting a segfault.
    delete g;
    if (plot.name.find("dead") != std::string::npos)
      countplots++;
  } // end loop on plots

  //
  // Create canvas with multiple trends - average threshold - 1 canvas per layer
  //
  ilay = 0;
  countplots = 0;
  TCanvas* c[NLAYERS * NTRENDSTHR];
  TLegend* legstaves[NLAYERS];
  for (int idx = 0; idx < NLAYERS * NTRENDSTHR; idx++) // define canvases
    c[idx] = new TCanvas(
      Form("threshold_%s_trends_L%d", trendnames[idx % NTRENDSTHR].c_str(),
           idx / NTRENDSTHR),
      Form("threshold_%s_trends_L%d", trendnames[idx % NTRENDSTHR].c_str(),
           idx / NTRENDSTHR));

  for (int ilay = 0; ilay < NLAYERS; ilay++) { // define legends
    legstaves[ilay] = new TLegend(0.91, 0.1, 0.98, 0.9);
    legstaves[ilay]->SetName(Form("legstaves_L%d", ilay));
    SetLegendStyle(legstaves[ilay]);
    PrepareLegend(legstaves[ilay], ilay);
  }
  ilay = 0;
  for (const auto& plot : mConfig.plots) {
    if (countplots > nStaves[ilay] - 1) {
      countplots = 0;
      ilay++;
    }
    int colidx = countplots > 13 ? countplots - 14
                                 : countplots > 6 ? countplots - 7 : countplots;
    int mkridx = countplots > 13 ? 2 : countplots > 6 ? 1 : 0;
    int add = (plot.name.find("rms") != std::string::npos)
                ? 1
                : plot.name.find("dead") != std::string::npos ? 2 : 0;

    bool isrun = plot.varexp.find("ntreeentries") != std::string::npos ? true : false; // vs run or vs time

    c[ilay * NTRENDSTHR + add]->cd();
    c[ilay * NTRENDSTHR + add]->SetTickx();
    c[ilay * NTRENDSTHR + add]->SetTicky();
    if (plot.name.find("dead") != std::string::npos)
      c[ilay * NTRENDSTHR + add]->SetLogy();
    long int n =
      mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), "goff");
    // post processing plot
    TGraph* g = new TGraph(n, mTrend->GetV2(), mTrend->GetV1());
    SetGraphStyle(g, col[colidx], mkr[mkridx]);
    double ymin = plot.name.find("rms") != std::string::npos
                    ? 0.
                    : plot.name.find("dead") != std::string::npos ? 1e-1 : 7.;
    double ymax = plot.name.find("rms") != std::string::npos
                    ? 5.
                    : plot.name.find("dead") != std::string::npos ? 5e3 : 14.;
    SetGraphNameAndAxes(g, plot.name,
                        Form("L%d - %s trends", ilay, trendtitles[add].c_str()),
                        isrun ? "run" : "time", ytitles[add], ymin, ymax, runlist);
    ILOG(Info, Support) << " Drawing " << plot.name << ENDM;

    if (!countplots && isrun) { //fake histo with runs as x-axis labels
      int npoints = g->GetN();
      TH1F* hfake = new TH1F("hfake", Form("%s; %s; %s", g->GetTitle(), g->GetXaxis()->GetTitle(), g->GetYaxis()->GetTitle()), npoints, 0.5, (double)npoints + 0.5);
      hfake->GetYaxis()->SetRangeUser(ymin, ymax);
      hfake->GetXaxis()->SetNdivisions(505);
      for (int ir = 0; ir < (int)runlist.size(); ir++)
        hfake->GetXaxis()->SetBinLabel(ir + 1, runlist[ir].c_str());
      hfake->DrawCopy();
      delete hfake;
    }

    g->DrawClone((!countplots && !isrun) ? plot.option.c_str() : Form("%s same", plot.option.c_str()));
    if (countplots == nStaves[ilay] - 1)
      legstaves[ilay]->Draw("same");
    if (plot.name.find("dead") != std::string::npos)
      countplots++;
  } // end loop on plots
  for (int idx = 0; idx < NLAYERS * NTRENDSTHR; idx++) {
    ILOG(Info, Support) << " Saving canvas for layer " << idx / NTRENDSTHR << " to CCDB "
                        << ENDM;
    auto mo = std::make_shared<MonitorObject>(c[idx], mConfig.taskName, "o2::quality_control_modules::its::TrendingTaskITSThr",
                                              mConfig.detectorName);
    mo->setIsOwner(false);
    qcdb.storeMO(mo);
    if (idx % NTRENDSTHR == NTRENDSTHR - 1)
      delete legstaves[idx / NTRENDSTHR];
    delete c[idx];
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

void TrendingTaskITSThr::SetGraphNameAndAxes(TGraph* g, std::string name,
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
    }
  }
}

void TrendingTaskITSThr::PrepareLegend(TLegend* leg, int layer)
{
  for (int istv = 0; istv < nStaves[layer]; istv++) {
    int colidx = istv > 13 ? istv - 14 : istv > 6 ? istv - 7 : istv;
    int mkridx = istv > 13 ? 2 : istv > 6 ? 1 : 0;
    TGraph* gr = new TGraph(); // dummy histo
    SetGraphStyle(gr, col[colidx], mkr[mkridx]);
    leg->AddEntry(gr, Form("%02d", istv), "pl");
  }
}
