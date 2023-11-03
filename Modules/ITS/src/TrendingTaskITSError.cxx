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
/// \file    TrendingTaskITSError.cxx
/// \author  Mariia Selina on the structure from Piotr Konopka
///

#include "ITS/TrendingTaskITSError.h"
#include "ITS/ReductorBinContent.h"

#include "QualityControl/RootClassFactory.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ObjectMetadataKeys.h"

#include <TList.h>
#include <TObject.h>
#include <TLegendEntry.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TFile.h>
#include <TDatime.h>
#include <map>
#include <string>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::repository;
using namespace o2::quality_control_modules::its;

void TrendingTaskITSError::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigITS(getID(), config);
}

void TrendingTaskITSError::initialize(Trigger, framework::ServiceRegistryRef)
{
  std::cout<<" at the biginning of initialize" << std::endl;
  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("Time", &mTime);
  mTrend->Branch("Entries", &nEntries);

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<ReductorBinContent> reductor(root_class_factory::create<ReductorBinContent>(source.moduleName, source.reductorName));
    std::cout<<" I am at the reducotr with source.name= " << source.name <<std::endl;
    if (source.name == "ChipErrorPlots")  
      reductor->setParams(o2::itsmft::ChipStat::NErrorsDefined);
    else reductor->setParams(o2::itsmft::GBTLinkDecodingStat::NErrorsDefined);
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }
  std::cout<<" at the end  of initialize" << std::endl;
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TrendingTaskITSError::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);
  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSError::finalize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  storePlots(qcdb);
  storeTrend(qcdb);
}

void TrendingTaskITSError::storeTrend(repository::DatabaseInterface& qcdb)
{
  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(), "o2::quality_control_modules::its::TrendingTaskITSError",
                                                  mConfig.detectorName, mMetaData.runNumber); // IVAN TEST
  mo->setIsOwner(false);
  qcdb.storeMO(mo);
}

void TrendingTaskITSError::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  std::cout<<" at the biginning of tremd values" << std::endl;
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
      auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
      if (mo == nullptr)
        continue;
      if (!count) {
        std::map<std::string, std::string> entryMetadata = mo->getMetadataMap();  // full list of metadata as a map
        mMetaData.runNumber = std::stoi(entryMetadata[metadata_keys::runNumber]); // get and set run number
        runlist.push_back(std::to_string(mMetaData.runNumber));
        nEntries = (Int_t)mTrend->GetEntriesFast() + 1;
      }
      TObject* obj = mo ? mo->getObject() : nullptr;
      if (obj)
        mReductors[dataSource.name]->update(obj);
    } else {
      ILOGE << "Unknown type of data source '" << dataSource.type << "'.";
    }
    count++;
  }
  mTrend->Fill();
  std::cout<<" at the end of trend values" << std::endl;

}

void TrendingTaskITSError::storePlots(repository::DatabaseInterface& qcdb)
{
  std::cout<<" at the biginning of store plots" << std::endl;
  // ILOG(Info, Support) << "Generating and storing " << mConfig.plots.size() << " plots." << ENDM;
  int countplots = 0;
  int countITSpart = 0;
  bool isRun = false;
  std::string name_Xaxis;
  long int numberOfEntries = mTrend->GetEntriesFast();
  // Define output graphs
 

  TString plots[2] = {"LinkErrorPlots", "ChipErrorPlots"};
  for (TString plotName: plots){

  TMultiGraph* multi_trend;
  countplots=0;
  // Lane status summary plots
  for (const auto& plot : mConfig.plots) {

     if (plot.varexp.find(plotName.Data()) == std::string::npos) continue;
     std::cout<<"plot.varexp"<< plot.varexp << " counts " << countplots<<std::endl; 

    // Initialize MultiGraph and Legend
    if (countplots == 0) {
      multi_trend = new TMultiGraph();
      SetGraphName(multi_trend, plot.name, Form("Trending plot of %s",plotName.Data()));

      isRun = plot.selection.find("Entries") != std::string::npos ? true : false;
      name_Xaxis = plot.selection.c_str();
    }

    // Retrieve data for trend plot
    mTrend->Draw(Form("%s:%s", name_Xaxis.c_str(), plot.varexp.c_str()), "", "goff");
    TGraph* trend_plot = new TGraph(numberOfEntries, mTrend->GetV1(), mTrend->GetV2());
    SetGraphStyle(trend_plot, plot.name, Form("ID%d", countplots), colors[countplots], markers[countplots]);
    multi_trend->Add((TGraph*)trend_plot->Clone(), plot.option.c_str());
    delete trend_plot;
    countplots++;
  }
  std::cout<<" survived loop, time to store " << std::endl;
  SetGraphAxes(multi_trend, Form("%s", isRun ? "Run" : "Time"), "Number of errors", !isRun);

  // Canvas settings
  std::string name;
  if (plotName == "LinkErrorPlots") name = "GTBLinkDecodingErrorsSummary_Trends";
     else name = "ChipDecodingErrorsSummary_Trends";

  std::cout<<"Will create canvas with name: " << name.c_str() << std::endl;
  TCanvas* canvas = new TCanvas(Form("%s", name.c_str()), Form("%s", name.c_str()));
  SetCanvasSettings(canvas);

  // Plot as a function of run number requires a dummy TH1 histogram
  TH1D* hDummy = nullptr;
  if (isRun) {
    hDummy = new TH1D("hDummy", Form("%s; %s; %s", multi_trend->GetTitle(), multi_trend->GetXaxis()->GetTitle(), multi_trend->GetYaxis()->GetTitle()), numberOfEntries, 0.5, numberOfEntries + 0.5);
    SetHistoAxes(hDummy, runlist, multi_trend->GetYaxis()->GetXmin(), multi_trend->GetYaxis()->GetXmax());
  }

  // Draw plots
  if (hDummy)
    hDummy->Draw();
  multi_trend->Draw(Form("%s", hDummy ? "" : "a"));

  TLegend* legend = (TLegend*)canvas->BuildLegend(0.77, 0.12, 1, 1);
  SetLegendStyle(legend, Form("%s_legend",plotName.Data()), isRun);
  legend->Draw("SAME");

  // Upload plots
  auto mo = std::make_shared<MonitorObject>(canvas, mConfig.taskName, "o2::quality_control_modules::its::TrendingTaskITSError", mConfig.detectorName, mMetaData.runNumber);
  mo->setIsOwner(false);
  qcdb.storeMO(mo);

  delete legend;
  delete canvas;
  delete multi_trend; // All included plots are deleted automatically
  delete hDummy;
  std::cout<<" survived store " << std::endl;
}
 std::cout<<" at the end of draw plots" << std::endl;
 
}
void TrendingTaskITSError::SetLegendStyle(TLegend* legend, const std::string& name, bool isRun)
{
  legend->SetTextFont(170);
  legend->SetLineColor(kWhite);
  legend->SetFillColor(0);
  legend->SetName(Form("%s_legend", name.c_str()));
  legend->SetFillStyle(0);
  legend->SetBorderSize(0);
  if (isRun)
    legend->GetListOfPrimitives()->Remove((TLegendEntry*)legend->GetListOfPrimitives()->First()); // Delete entry from dummy histogram
}

void TrendingTaskITSError::SetCanvasSettings(TCanvas* canvas)
{
  canvas->SetTickx();
  canvas->SetTicky();
  canvas->SetBottomMargin(0.18);
  canvas->SetRightMargin(0.23);
}

void TrendingTaskITSError::SetGraphStyle(TGraph* graph, const std::string& name, const std::string& title, int col, int mkr)
{
  graph->SetName(Form("%s", name.c_str()));
  graph->SetTitle(Form("%s", title.c_str()));
  graph->SetLineColor(col);
  graph->SetMarkerStyle(mkr);
  graph->SetMarkerColor(col);
  graph->SetMarkerSize(1.0);
  graph->SetLineWidth(1);
}

void TrendingTaskITSError::SetGraphName(TMultiGraph* graph, const std::string& name, const std::string& title)
{
  graph->SetTitle(title.c_str());
  graph->SetName(name.c_str());
}

void TrendingTaskITSError::SetGraphAxes(TMultiGraph* graph, const std::string& xtitle,
                                        const std::string& ytitle, bool isTime)
{
  graph->GetXaxis()->SetTitle(xtitle.c_str());
  graph->GetXaxis()->SetTitleSize(0.045);

  graph->GetYaxis()->SetTitle(ytitle.c_str());
  graph->GetYaxis()->SetTitleSize(0.045);

  graph->GetXaxis()->SetNdivisions(505);
  graph->GetXaxis()->SetTimeOffset(0.0);
  graph->GetXaxis()->SetLabelOffset(0.02);
  graph->GetXaxis()->SetLabelSize(0.033);
  graph->GetXaxis()->SetTitleOffset(2.3);
  if (isTime) {
    graph->GetXaxis()->SetTimeFormat("#splitline{%d.%m.%y}{%H:%M}");
    graph->GetXaxis()->SetTimeDisplay(1);
  }
}

void TrendingTaskITSError::SetHistoAxes(TH1* hist, const std::vector<std::string>& runlist,
                                        const double& Ymin, const double& Ymax)
{
  hist->SetStats(0);
  hist->GetXaxis()->SetTitleSize(0.045);
  hist->GetYaxis()->SetTitleSize(0.045);
  hist->GetYaxis()->SetRangeUser(Ymin, Ymax);

  hist->GetXaxis()->SetLabelOffset(0.02);
  hist->GetXaxis()->SetLabelSize(0.033);
  hist->GetXaxis()->SetTitleOffset(2.3);
  hist->GetXaxis()->SetNdivisions(-505);

  for (int iX = 0; iX < runlist.size(); iX++) {
    hist->GetXaxis()->SetBinLabel(iX + 1, runlist[iX].c_str());
  }
}
