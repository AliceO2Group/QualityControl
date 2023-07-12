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
/// \file     SliceTrendingTask.cxx
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#include "QualityControl/SliceTrendingTask.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/ActivityHelpers.h"

#include <string>
#include <TGraphErrors.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>
#include <fmt/format.h>
#include <TAxis.h>
#include <TH2F.h>
#include <TStyle.h>
#include <TMultiGraph.h>
#include <TIterator.h>
#include <TLegend.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

void SliceTrendingTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = SliceTrendingTaskConfig(getID(), config);
}

void SliceTrendingTask::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // Prepare the data structure of the trending TTree.
  if (mConfig.resumeTrend) {
    ILOG(Info, Support) << "Trying to retrieve an existing TTree for this task to continue the trend." << ENDM;
    auto& qcdb = services.get<repository::DatabaseInterface>();
    auto path = RepoPathUtils::getMoPath(mConfig.detectorName, PostProcessingInterface::getName(), "", "", false);
    auto mo = qcdb.retrieveMO(path, PostProcessingInterface::getName(), -1, mConfig.activity);
    if (mo && mo->getObject()) {
      auto tree = dynamic_cast<TTree*>(mo->getObject());
      if (tree) {
        mTrend = std::unique_ptr<TTree>(tree);
        mo->setIsOwner(false);
      }
    } else {
      ILOG(Warning, Support) << "Could not retrieve an existing TTree for this task, maybe there is none which match these Activity settings" << ENDM;
    }
  }
  if (mTrend == nullptr) {
    ILOG(Info, Support) << "Generating new TTree for SliceTrending" << ENDM;
    mTrend = std::make_unique<TTree>();
    mTrend->SetName(PostProcessingInterface::getName().c_str());

    mTrend->Branch("meta", &mMetaData, "runNumber/I");
    mTrend->Branch("time", &mTime);
    for (const auto& source : mConfig.dataSources) {
      mSources[source.name] = new std::vector<SliceInfo>();
      mTrend->Branch(source.name.c_str(), &mSources[source.name]);
    }
  } else {                                                    // we picked up an older TTree
    mTrend->SetBranchAddress("meta", &(mMetaData.runNumber)); // TO-DO: Find reason why simply &mMetaData does not work
    mTrend->SetBranchAddress("time", &mTime);
    for (const auto& source : mConfig.dataSources) {
      bool existingBranch = mTrend->GetBranchStatus(source.name.c_str());
      mSources[source.name] = new std::vector<SliceInfo>();
      if (existingBranch) {
        mTrend->SetBranchAddress(source.name.c_str(), &mSources[source.name]);
      } else {
        mTrend->Branch(source.name.c_str(), &mSources[source.name]);
      }
    }
  }
  // Reductors
  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<SliceReductor> reductor(root_class_factory::create<SliceReductor>(
      source.moduleName, source.reductorName));
    mReductors[source.name] = std::move(reductor);
  }

  if (mConfig.producePlotsOnUpdate) {
    getObjectsManager()->startPublishing(mTrend.get());
  }
}

void SliceTrendingTask::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  trendValues(t, qcdb);
  if (mConfig.producePlotsOnUpdate) {
    generatePlots();
  }
}

void SliceTrendingTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
  if (!mConfig.producePlotsOnUpdate) {
    getObjectsManager()->startPublishing(mTrend.get());
  }
  generatePlots();

  for (const auto& source : mConfig.dataSources) {
    delete mSources[source.name];
    mSources[source.name] = nullptr;
  }
}

void SliceTrendingTask::trendValues(const Trigger& t,
                                    repository::DatabaseInterface& qcdb)
{
  mTime = activity_helpers::isLegacyValidity(t.activity.mValidity)
            ? t.timestamp / 1000
            : t.activity.mValidity.getMax() / 1000; // ROOT expects seconds since epoch.
  mMetaData.runNumber = t.activity.mId;
  for (auto& dataSource : mConfig.dataSources) {
    mNumberPads[dataSource.name] = 0;
    mSources[dataSource.name]->clear();
    if (dataSource.type == "repository") {
      auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
      TObject* obj = mo ? mo->getObject() : nullptr;

      mAxisDivision[dataSource.name] = dataSource.axisDivision;

      if (obj) {
        mReductors[dataSource.name]->update(obj, *mSources[dataSource.name],
                                            dataSource.axisDivision, mNumberPads[dataSource.name]);
      }

    } else {
      ILOG(Error, Support) << "Data source '" << dataSource.type << "' is not of type repository." << ENDM;
    }
  }

  mTrend->Fill();
} // void SliceTrendingTask::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)

void SliceTrendingTask::generatePlots()
{
  if (mTrend == nullptr) {
    ILOG(Info, Support) << "The trend object is not there, won't generate any plots." << ENDM;
    return;
  }

  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, no plot generated." << ENDM;
    return;
  }

  ILOG(Info, Support) << "Generating " << mConfig.plots.size() << " plots." << ENDM;
  for (const auto& plot : mConfig.plots) {
    // Delete the existing plots before regenerating them.
    if (mPlots.count(plot.name)) {
      getObjectsManager()->stopPublishing(plot.name);
      delete mPlots[plot.name];
    }

    // Postprocess each pad (titles, axes, flushing buffers).
    const std::size_t posEndVar = plot.varexp.find('.'); // Find the end of the dataSource.
    const std::string varName(plot.varexp.substr(0, posEndVar));

    // Draw the trending on a new canvas.
    auto* c = new TCanvas();
    c->SetName(plot.name.c_str());
    c->SetTitle(plot.title.c_str());

    drawCanvasMO(c, plot.varexp, plot.name, plot.option, plot.graphErrors, mAxisDivision[varName]);

    int NumberPlots = 1;
    if (plot.varexp.find(":time") != std::string::npos || plot.varexp.find(":run") != std::string::npos) { // we plot vs time, multiple plots on canvas possible
      NumberPlots = mNumberPads[varName];
    }
    for (int p = 0; p < NumberPlots; p++) {
      c->cd(p + 1);
      if (auto histo = dynamic_cast<TGraphErrors*>(c->cd(p + 1)->GetPrimitive("Graph"))) {
        beautifyGraph(histo, plot, c);
        // Manually empty the buffers before visualising the plot.
        // histo->BufferEmpty(); // TBD: Should we keep it or not? Graph does not have this method.c
      } else if (auto multigraph = dynamic_cast<TMultiGraph*>(c->cd(p + 1)->GetPrimitive("MultiGraph"))) {
        if (auto legend = dynamic_cast<TLegend*>(c->cd(2)->GetPrimitive("MultiGraphLegend"))) {
          c->cd(1);
          beautifyGraph(multigraph, plot, c);
          c->cd(1)->SetLeftMargin(0.15);
          c->cd(1)->SetRightMargin(0.01);
          c->cd(2)->SetLeftMargin(0.01);
          c->cd(2)->SetRightMargin(0.01);
        } else {
          ILOG(Error, Support) << "No legend in multigraph-time" << ENDM;
          c->cd(1);
          beautifyGraph(multigraph, plot, c);
        }
        c->Modified();
        c->Update();
      } else if (auto histo = dynamic_cast<TH2F*>(c->cd(p + 1)->GetPrimitive("Graph2D"))) {

        const std::string thisTitle = fmt::format("{0:s}", plot.title.data());
        histo->SetTitle(thisTitle.data());

        if (!plot.graphAxisLabel.empty()) {
          setUserAxisLabel(histo->GetXaxis(), histo->GetYaxis(), plot.graphAxisLabel);
          c->Modified();
          c->Update();
        }

        if (!plot.graphYRange.empty()) {
          float yMin, yMax;
          getUserAxisRange(plot.graphYRange, yMin, yMax);
          histo->SetMinimum(yMin);
          histo->SetMaximum(yMax);
          c->Modified();
          c->Update();
        }

        gStyle->SetPalette(kBird);
        histo->SetStats(kFALSE);
      } else {
        ILOG(Error, Devel) << "Could not get the 'Graph' of the plot '"
                           << plot.name << "'." << ENDM;
      }
    }

    mPlots[plot.name] = c;
    getObjectsManager()->startPublishing(c);
  }
} // void SliceTrendingTask::generatePlots()

void SliceTrendingTask::drawCanvasMO(TCanvas* thisCanvas, const std::string& var,
                                     const std::string& name, const std::string& opt, const std::string& err, const std::vector<std::vector<float>>& axis)
{
  // Determine the order of the plot (1 - histo, 2 - graph, ...)
  const size_t plotOrder = std::count(var.begin(), var.end(), ':') + 1;

  // Prepare the strings for the dataSource and its trending quantity.
  std::string varName, typeName, trendType;
  getTrendVariables(var, varName, typeName, trendType);

  std::string errXName, errYName;
  getTrendErrors(err, errXName, errYName);

  // Divide the canvas into the correct number of pads.
  if (trendType == "time" || trendType == "run") {
    thisCanvas->DivideSquare(mNumberPads[varName]); // trending vs time: multiple plots per canvas possible
  } else if (trendType == "multigraphtime" || trendType == "multigraphrun") {
    thisCanvas->Divide(2, 1);
  } else {
    thisCanvas->DivideSquare(1);
  }

  // Delete the graph errors after the plot is saved. //To-Do check if ownership is now taken
  // Unfortunately the canvas does not take its ownership.
  TGraphErrors* graphErrors = nullptr;

  // Setup the tree reader with the needed values.
  TTreeReader myReader(mTrend.get());
  TTreeReaderValue<UInt_t> retrieveTime(myReader, "time");
  TTreeReaderValue<Int_t> retrieveRun(myReader, "meta.runNumber");
  TTreeReaderValue<std::vector<SliceInfo>> dataRetrieveVector(myReader, varName.data());

  const int nuPa = mNumberPads[varName];
  const int nEntries = mTrend->GetEntriesFast();
  const int nEntriesTime = mTrend->GetBranch("time")->GetEntries();
  const int nEntriesRuns = mTrend->GetBranch("meta")->GetEntries();
  const int nEntriesData = mTrend->GetBranch(varName.data())->GetEntries();

  // Fill the graph(errors) to be published.
  if (trendType == "time" || trendType == "run") {

    const int nEffectiveEntries = (trendType == "time") ? std::min(nEntriesTime, nEntriesData) : std::min(nEntriesRuns, nEntriesData);
    const int startPoint = (trendType == "time") ? nEntriesTime - nEffectiveEntries : nEntriesRuns - nEffectiveEntries;

    for (int p = 0; p < nuPa; p++) {
      thisCanvas->cd(p + 1);
      int iEntry = 0;
      graphErrors = new TGraphErrors(nEffectiveEntries);
      myReader.SetEntry(startPoint - 1); // startPoint-1 as myReader.Next() increments by one so that we then start at startPoint

      while (myReader.Next()) {
        const double timeStamp = (trendType == "time") ? (double)(*retrieveTime) : (double)(*retrieveRun);
        const double dataPoint = (dataRetrieveVector->at(p)).retrieveValue(typeName);
        double errorX = 0.;
        double errorY = 0.;

        if (!err.empty()) {
          errorX = (dataRetrieveVector->at(p)).retrieveValue(errXName);
          errorY = (dataRetrieveVector->at(p)).retrieveValue(errYName);
        }

        graphErrors->SetPoint(iEntry, timeStamp, dataPoint);
        graphErrors->SetPointError(iEntry, errorX, errorY); // Add Error to the last added point

        iEntry++;
      }
      graphErrors->SetTitle((dataRetrieveVector->at(p)).title.data());
      myReader.Restart();

      if (!err.empty()) {
        if (plotOrder != 2) {
          ILOG(Info, Support) << "Non empty graphErrors seen for the plot '" << name
                              << "', which is not a graph, ignoring." << ENDM;
        } else {
          graphErrors->Draw(opt.data());
        }
      }
    }
  } // Trending vs time
  else if (trendType == "multigraphtime" || trendType == "multigraphrun") {

    auto multigraph = new TMultiGraph();
    multigraph->SetName("MultiGraph");

    const int nEffectiveEntries = (trendType == "multigraphtime") ? std::min(nEntriesTime, nEntriesData) : std::min(nEntriesRuns, nEntriesData);
    const int startPoint = (trendType == "multigraphtime") ? nEntriesTime - nEffectiveEntries : nEntriesRuns - nEffectiveEntries;

    for (int p = 0; p < nuPa; p++) {
      int iEntry = 0;
      auto gr = new TGraphErrors(nEffectiveEntries);
      myReader.SetEntry(startPoint - 1); // startPoint-1 as myReader.Next() increments by one so that we then start at startPoint

      while (myReader.Next()) {
        const double timeStamp = (trendType == "multigraphtime") ? (double)(*retrieveTime) : (double)(*retrieveRun);
        const double dataPoint = (dataRetrieveVector->at(p)).retrieveValue(typeName);
        double errorX = 0.;
        double errorY = 0.;

        if (!err.empty()) {
          errorX = (dataRetrieveVector->at(p)).retrieveValue(errXName);
          errorY = (dataRetrieveVector->at(p)).retrieveValue(errYName);
        }

        gr->SetPoint(iEntry, timeStamp, dataPoint);
        gr->SetPointError(iEntry, errorX, errorY); // Add Error to the last added point
        iEntry++;
      }

      const std::string_view title = (dataRetrieveVector->at(p)).title;
      const auto posDivider = title.find("RangeX");
      if (posDivider != std::string_view::npos) {
        gr->SetName(title.substr(posDivider, -1).data());
      } else {
        gr->SetName(title.data());
      }

      myReader.Restart();
      multigraph->Add(gr);
    } // for (int p = 0; p < nuPa; p++)

    thisCanvas->cd(1);
    multigraph->Draw("A pmc plc");

    auto legend = new TLegend(0., 0.1, 0.95, 0.9);
    legend->SetName("MultiGraphLegend");
    legend->SetNColumns(2);
    legend->SetTextSize(2.0);
    for (auto obj : *multigraph->GetListOfGraphs()) {
      legend->AddEntry(obj, obj->GetName(), "lpf");
    }
    thisCanvas->cd(2);
    legend->Draw();

  } // Trending vs Time as Multigraph
  else if (trendType == "slices") {

    graphErrors = new TGraphErrors(nuPa);
    thisCanvas->cd(1);

    myReader.SetEntry(nEntries - 1); // set event to last entry with index nEntries-1

    int iEntry = 0;
    for (int p = 0; p < nuPa; p++) {

      const double dataPoint = (dataRetrieveVector->at(p)).retrieveValue(typeName);
      double errorX = 0.;
      double errorY = 0.;
      if (!err.empty()) {
        errorX = (dataRetrieveVector->at(p)).retrieveValue(errXName);
        errorY = (dataRetrieveVector->at(p)).retrieveValue(errYName);
      }
      const double xLabel = (dataRetrieveVector->at(p)).retrieveValue("sliceLabelX");

      graphErrors->SetPoint(iEntry, xLabel, dataPoint);
      graphErrors->SetPointError(iEntry, errorX, errorY); // Add Error to the last added point

      iEntry++;
    }

    if (myReader.Next()) {
      ILOG(Error, Devel) << "Entry beyond expected last entry" << ENDM;
    }

    myReader.Restart();

    if (!err.empty()) {
      if (plotOrder != 2) {
        ILOG(Info, Support) << "Non empty graphErrors seen for the plot '" << name
                            << "', which is not a graph, ignoring." << ENDM;
      } else {
        graphErrors->Draw(opt.data());
      }
    }
  } // Trending vs Slices
  else if (trendType == "slices2D") {

    thisCanvas->cd(1);
    const int xBins = axis[0].size();
    float xBoundaries[xBins];
    for (int i = 0; i < xBins; i++) {
      xBoundaries[i] = axis[0][i];
    }
    const int yBins = axis[1].size();
    float yBoundaries[yBins];
    for (int i = 0; i < yBins; i++) {
      yBoundaries[i] = axis[1][i];
    }

    TH2F* graph2D = new TH2F("", "", xBins - 1, xBoundaries, yBins - 1, yBoundaries);
    graph2D->SetName("Graph2D");
    thisCanvas->cd(1);
    myReader.SetEntry(nEntries - 1); // set event to last entry with index nEntries-1

    int iEntry = 0;
    for (int p = 0; p < nuPa; p++) {

      const double dataPoint = (double)(dataRetrieveVector->at(p)).retrieveValue(typeName);
      double error = 0.;
      if (!err.empty()) {
        error = (double)(dataRetrieveVector->at(p)).retrieveValue(errYName);
      }
      const double xLabel = (double)(dataRetrieveVector->at(p)).retrieveValue("sliceLabelX");
      const double yLabel = (double)(dataRetrieveVector->at(p)).retrieveValue("sliceLabelY");

      graph2D->Fill(xLabel, yLabel, dataPoint);
      graph2D->SetBinError(graph2D->GetXaxis()->FindBin(xLabel), graph2D->GetYaxis()->FindBin(yLabel), error);

      iEntry++;
    }

    if (myReader.Next()) {
      ILOG(Error, Devel) << "Entry beyond expected last entry" << ENDM;
    }

    myReader.Restart();
    gStyle->SetPalette(kBird);
    graph2D->Draw(opt.data());
  } // Trending vs Slices2D
}

void SliceTrendingTask::getUserAxisRange(const std::string& graphAxisRange, float& limitLow, float& limitUp)
{
  const std::size_t posDivider = graphAxisRange.find(':');
  const std::string minString(graphAxisRange.substr(0, posDivider));
  const std::string maxString(graphAxisRange.substr(posDivider + 1));

  limitLow = std::stof(minString);
  limitUp = std::stof(maxString);
}

void SliceTrendingTask::setUserAxisLabel(TAxis* xAxis, TAxis* yAxis, const std::string& graphAxisLabel)
{
  const std::size_t posDivider = graphAxisLabel.find(':');
  const std::string yLabel(graphAxisLabel.substr(0, posDivider));
  const std::string xLabel(graphAxisLabel.substr(posDivider + 1));

  xAxis->SetTitle(xLabel.data());
  yAxis->SetTitle(yLabel.data());
}

void SliceTrendingTask::getTrendVariables(const std::string& inputvar, std::string& sourceName, std::string& variableName, std::string& trend)
{
  const std::size_t posEndVar = inputvar.find('.');  // Find the end of the dataSource.
  const std::size_t posEndType = inputvar.find(':'); // Find the end of the quantity.
  sourceName = inputvar.substr(0, posEndVar);
  variableName = inputvar.substr(posEndVar + 1, posEndType - posEndVar - 1);
  trend = inputvar.substr(posEndType + 1, -1);
}

void SliceTrendingTask::getTrendErrors(const std::string& inputvar, std::string& errorX, std::string& errorY)
{
  const std::size_t posEndType_err = inputvar.find(':'); // Find the end of the error.
  errorX = inputvar.substr(posEndType_err + 1);
  errorY = inputvar.substr(0, posEndType_err);
}

template <typename T>
void SliceTrendingTask::beautifyGraph(T& graph, const SliceTrendingTaskConfig::Plot& plotconfig, TCanvas* canv)
{

  // Set the title of the graph in a proper way.
  std::string thisTitle;
  if (plotconfig.varexp.find(":time") != std::string::npos) {
    thisTitle = fmt::format("{0:s} - {1:s}", plotconfig.title.data(), graph->GetTitle()); // for plots vs time slicing might be applied for the title
  } else {
    thisTitle = fmt::format("{0:s}", plotconfig.title.data());
  }
  graph->SetTitle(thisTitle.data());

  // Set the user-defined range on the y axis if needed.
  if (!plotconfig.graphYRange.empty()) {
    float yMin, yMax;
    getUserAxisRange(plotconfig.graphYRange, yMin, yMax);
    graph->SetMinimum(yMin);
    graph->SetMaximum(yMax);
    canv->Modified();
    canv->Update();
  }

  if (!plotconfig.graphXRange.empty()) {
    float xMin, xMax;
    getUserAxisRange(plotconfig.graphXRange, xMin, xMax);
    graph->GetXaxis()->SetLimits(xMin, xMax);
    canv->Modified();
    canv->Update();
  }

  if (!plotconfig.graphAxisLabel.empty()) {
    setUserAxisLabel(graph->GetXaxis(), graph->GetYaxis(), plotconfig.graphAxisLabel);
    canv->Modified();
    canv->Update();
  }

  // Configure the time for the x axis.
  if (plotconfig.varexp.find(":time") != std::string::npos || plotconfig.varexp.find(":multigraphtime") != std::string::npos) {
    graph->GetXaxis()->SetTimeDisplay(1);
    graph->GetXaxis()->SetNdivisions(505);
    graph->GetXaxis()->SetTimeOffset(0.0);
    graph->GetXaxis()->SetLabelOffset(0.02);
    graph->GetXaxis()->SetTimeFormat("#splitline{%d.%m.%y}{%H:%M}");
  } else if (plotconfig.varexp.find(":meta.runNumber") != std::string::npos || plotconfig.varexp.find(":run") != std::string::npos || plotconfig.varexp.find(":multigraphrun") != std::string::npos) {
    graph->GetXaxis()->SetNoExponent(true);
  }
}
