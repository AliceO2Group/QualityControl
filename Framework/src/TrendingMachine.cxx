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
/// \file    TrendingMachine.cxx
/// \author  Piotr Konopka
///
/// \brief   Implementation of trend generation functionality
///
/// This class contains code extracted from TrendingTask to eliminate duplication between
/// TrendingTask and LateTrendingTask.
///
/// Code history can be accessed with:
///   git log --follow Framework/src/TrendingMachine.cxx
///   git blame -C -C -C Framework/src/TrendingMachine.cxx
///

#include "QualityControl/TrendingMachine.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/RootClassFactory.h"

#include <TTree.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TLegend.h>
#include <TAxis.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TPoint.h>

#include <boost/algorithm/string.hpp>
#include <set>
#include <stdexcept>
#include <utility>

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;

TrendingMachine::TrendingMachine(const TrendingMachineConfig& config)
  : mPlotConfigs(config.plots), mDataSources(config.dataSources)
{
  // Create reductors
  for (const auto& source : mDataSources) {
    auto&& [emplaced, _] = mReductors.emplace(source.name, core::root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
    if (!emplaced->second) {
      throw std::runtime_error("Failed to create reductor " + source.reductorName + " from module " + source.moduleName);
    }
    emplaced->second->setCustomConfig(source.reductorParameters);
  }
}

bool TrendingMachine::initializeTrend(const std::string& treeName, std::unique_ptr<TTree> existingTree)
{
  mTrend.reset();

  // If we have an existing tree and can continue it, resume from it
  if (existingTree && canContinueTrend(existingTree.get(), mDataSources)) {
    ILOG(Info, Support) << "Will continue the trend from the provided tree." << ENDM;
    mTrend = std::move(existingTree);
    mTrend->SetBranchAddress("meta", &mMetaData);
    mTrend->SetBranchAddress("time", &mTime);
    for (const auto& [sourceName, reductor] : mReductors) {
      mTrend->SetBranchAddress(sourceName.c_str(), reductor->getBranchAddress());
    }
    return true;
  }

  // did not manage to reuse existingTree, creating a new one
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(treeName.c_str());

  mTrend->Branch("meta", &mMetaData, mMetaData.getBranchLeafList());
  mTrend->Branch("time", &mTime);
  for (const auto& [sourceName, reductor] : mReductors) {
    mTrend->Branch(sourceName.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
  }
  return false;
}

void TrendingMachine::setMetadata(UInt_t timestamp, int runNumber)
{
  mTime = timestamp;
  mMetaData.runNumber = runNumber;
  std::snprintf(mMetaData.runNumberStr, MaxRunNumberStringLength + 1, "%d", runNumber);
}

const Reductor* TrendingMachine::getReductor(const std::string& dataSourceName) const
{
  auto it = mReductors.find(dataSourceName);
  return it != mReductors.end() ? it->second.get() : nullptr;
}

bool TrendingMachine::updateReductors(const std::function<bool(const TrendingMachineConfig::DataSource&, Reductor&)>& updater)
{
  bool wereAllSourcesUpdated = true;
  for (const auto& dataSource : mDataSources) {
    auto it = mReductors.find(dataSource.name);
    if (it == mReductors.end()) {
      wereAllSourcesUpdated = false;
      ILOG(Error, Support) << "Reductor not found for data source '" << dataSource.name << "'." << ENDM;
      continue;
    }
    auto reductor = it->second.get();
    if (!updater(dataSource, *reductor)) {
      wereAllSourcesUpdated = false;
      ILOG(Error, Support) << "Failed to update reductor for data sources with path '" << dataSource.path
                           << "', name '" << dataSource.name
                           << "', type '" << dataSource.type << "'." << ENDM;
    }
  }
  return wereAllSourcesUpdated;
}

void TrendingMachine::fillTrend()
{
  if (mTrend) {
    mTrend->Fill();
  }
}

std::vector<std::unique_ptr<TCanvas>> TrendingMachine::generatePlots()
{
  std::vector<std::unique_ptr<TCanvas>> plots;

  if (mTrend == nullptr) {
    ILOG(Info, Support) << "The trend object is not there, won't generate any plots." << ENDM;
    return plots;
  }

  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, won't generate any plots." << ENDM;
    return plots;
  }

  ILOG(Info, Support) << "Generating " << mPlotConfigs.size() << " plots." << ENDM;
  for (const auto& plotConfig : mPlotConfigs) {
    plots.emplace_back(drawPlot(mTrend.get(), plotConfig));
  }

  return plots;
}

void TrendingMachine::reset()
{
  if (mTrend) {
    mTrend->Reset();
  }
}

TCanvas* TrendingMachine::drawPlot(TTree* trend, const TrendingMachineConfig::Plot& plotConfig)
{
  auto* c = new TCanvas();

  // Legend
  TLegend* legend = nullptr;
  if (plotConfig.legend.x1 >= 0 && plotConfig.legend.y1 >= 0 && plotConfig.legend.x2 >= 0 && plotConfig.legend.y2 >= 0) {
    legend = new TLegend(plotConfig.legend.x1, plotConfig.legend.y1,
                         plotConfig.legend.x2, plotConfig.legend.y2,
                         nullptr, "NDC");
    if (plotConfig.legend.nColumns > 0) {
      legend->SetNColumns(plotConfig.legend.nColumns);
    }
  } else {
    legend = new TLegend(0.3, 0.2);
  }
  legend->SetBorderSize(0);
  legend->SetFillStyle(0);
  legend->SetTextSize(0.03);
  legend->SetMargin(0.15);

  // Keep palette behavior unless user forces explicit colors via per-graph style
  if (plotConfig.colorPalette != 0) {
    gStyle->SetPalette(plotConfig.colorPalette);
    // This makes ROOT store the selected palette for each generated plot.
    // TColor::DefinedColors(1); // TODO enable when available
  } else {
    gStyle->SetPalette(); // default
  }

  // regardless whether we draw a graph or a histogram, a histogram is always used by TTree::Draw to draw axes and title
  // we attempt to keep it to do some modifications later
  TH1* background = nullptr;
  bool firstGraphInPlot = true;
  // by "graph" we consider anything we can draw, not necessarily TGraph, and we draw all on the same canvas
  for (const auto& graphConfig : plotConfig.graphs) {
    // we determine the order of the plotConfig, i.e. if it is a histogram (1), graphConfig (2), or any higher dimension.
    const size_t plotOrder = std::count(graphConfig.varexp.begin(), graphConfig.varexp.end(), ':') + 1;

    // having "SAME" at the first TTree::Draw() call will not work, we have to add it only in subsequent Draw calls
    std::string option = firstGraphInPlot ? graphConfig.option : "SAME " + graphConfig.option;

    // Draw main series
    trend->Draw(graphConfig.varexp.c_str(), graphConfig.selection.c_str(), option.c_str());

    // For graphs, we allow to draw errors if they are specified.
    TGraphErrors* graphErrors = nullptr;
    if (!graphConfig.errors.empty()) {
      if (plotOrder != 2) {
        ILOG(Error, Support) << "Non empty graphErrors seen for the plotConfig '" << plotConfig.name << "', which is not a graphConfig, ignoring." << ENDM;
      } else {
        // We generate some 4-D points, where 2 dimensions represent graph points and 2 others are the error bars
        std::string varexpWithErrors(graphConfig.varexp + ":" + graphConfig.errors);
        trend->Draw(varexpWithErrors.c_str(), graphConfig.selection.c_str(), "goff");
        graphErrors = new TGraphErrors(trend->GetSelectedRows(), trend->GetVal(1), trend->GetVal(0),
                                       trend->GetVal(2), trend->GetVal(3));
        graphErrors->SetName((graphConfig.name + "_errors").c_str());
        graphErrors->SetTitle((graphConfig.title + " errors").c_str());
        // We draw on the same plotConfig as the main graphConfig, but only error bars
        graphErrors->Draw("SAME E");
      }
    }

    // Legend entry and styling for graphs
    if (auto graph = dynamic_cast<TGraph*>(c->FindObject("Graph"))) {
      if (plotOrder >= 2) {
        // Style objects after Draw so we override palette/auto styling when requested
        applyStyleToGraph(graph, graphConfig.style);
        // Keep errors visually consistent with the main series
        if (graphErrors) {
          applyStyleToGraph(graphErrors, graphConfig.style);
        }
      }
      graph->SetName(graphConfig.name.c_str());
      graph->SetTitle(graphConfig.title.c_str());
      legend->AddEntry(graph, graphConfig.title.c_str(),
                       deduceGraphLegendOptions(graphConfig).c_str());
    }

    // Legend entry and styling for histograms
    if (auto htemp = dynamic_cast<TH1*>(c->FindObject("htemp"))) {
      if (plotOrder == 1) {
        htemp->SetName(graphConfig.name.c_str());
        htemp->SetTitle(graphConfig.title.c_str());
        legend->AddEntry(htemp, graphConfig.title.c_str(), "lpf");
      } else {
        htemp->SetName("background");
        htemp->SetTitle("background");
        // htemp was used by TTree::Draw only to draw axes and title, not to plot data, no need to add it to legend
      }
      // QCG doesn't empty the buffers before visualizing the plotConfig, nor does ROOT when saving the file,
      // so we have to do it here.
      htemp->BufferEmpty();
      // we keep the pointer to bg histogram for later postprocessing
      if (!background) {
        background = htemp;
      }
    }

    firstGraphInPlot = false;
  }

  c->SetName(plotConfig.name.c_str());
  c->SetTitle(plotConfig.title.c_str());

  // Postprocessing the plotConfig - adding specified titles, configuring time-based plots, flushing buffers.
  // Notice that axes and title are drawn using a histogram, even in the case of graphs.
  if (background) {
    // The title of background histogram is printed, not the title of canvas => we set it as well.
    background->SetTitle(plotConfig.title.c_str());
    // We have to update the canvas to make the title appear and access it in the next step.
    c->Update();

    // After the update, the title has a different size and it is not in the center anymore. We have to fix that.
    if (auto title = dynamic_cast<TPaveText*>(c->GetPrimitive("title"))) {
      title->SetBBoxCenterX(c->GetBBoxCenter().fX);
      c->Modified();
      c->Update();
    } else {
      ILOG(Error, Devel) << "Could not get the title TPaveText of the plotConfig '" << plotConfig.name << "'." << ENDM;
    }

    if (!plotConfig.graphAxisLabel.empty()) {
      setUserAxesLabels(background->GetXaxis(), background->GetYaxis(), plotConfig.graphAxisLabel);
    }

    if (plotConfig.graphs.back().varexp.find(":time") != std::string::npos) {
      // We have to explicitly configure showing time on x axis.
      formatTimeXAxis(background);
    } else if (plotConfig.graphs.back().varexp.find(":meta.runNumber") != std::string::npos) {
      formatRunNumberXAxis(background);
    }

    // Set the user-defined range on the y axis if needed.
    if (!plotConfig.graphYRange.empty()) {
      setUserYAxisRange(background, plotConfig.graphYRange);
      c->Modified();
      c->Update();
    }
  } else {
    ILOG(Error, Devel) << "Could not get the htemp histogram of the plotConfig '" << plotConfig.name << "'." << ENDM;
  }

  if (plotConfig.graphs.size() > 1) {
    legend->Draw();
  } else {
    delete legend;
  }

  c->Modified();
  c->Update();

  return c;
}

void TrendingMachine::applyStyleToGraph(TGraph* graph, const TrendingMachineConfig::GraphStyle& style)
{
  if (!graph) {
    return;
  }

  if (style.lineColor >= 0) {
    graph->SetLineColor(style.lineColor);
  }
  if (style.lineStyle >= 0) {
    graph->SetLineStyle(style.lineStyle);
  }
  if (style.lineWidth >= 0) {
    graph->SetLineWidth(style.lineWidth);
  }

  if (style.markerColor >= 0) {
    graph->SetMarkerColor(style.markerColor);
  }
  if (style.markerStyle >= 0) {
    graph->SetMarkerStyle(style.markerStyle);
  }
  if (style.markerSize >= 0.f) {
    graph->SetMarkerSize(style.markerSize);
  }

  if (style.fillColor >= 0) {
    graph->SetFillColor(style.fillColor);
  }
  if (style.fillStyle >= 0) {
    graph->SetFillStyle(style.fillStyle);
  }
}

std::string TrendingMachine::deduceGraphLegendOptions(const TrendingMachineConfig::Graph& graphConfig)
{
  // Looking at TGraphPainter documentation, the number of TGraph options is rather small,
  // so we can try to be smart and deduce the corresponding legend options.
  // I am not aware of any ROOT helper which can do this.
  std::string options = graphConfig.option;
  boost::algorithm::to_lower(options);
  // these three options have only an influence on colours but not what is drawn and what not.
  for (const auto& toRemove : { "pfc", "plc", "pmc" }) {
    if (size_t pos = options.find(toRemove); pos != std::string::npos) {
      options.erase(pos, 3);
    }
  }
  auto optionsHave = [&](std::string_view seq) {
    return options.find(seq) != std::string::npos;
  };

  std::string out;
  if (optionsHave("l") || optionsHave("c")) {
    out += "l"; // line
  }
  if (optionsHave("*") || optionsHave("p")) {
    out += "p"; // point
  }
  if (optionsHave("f") || optionsHave("b")) {
    out += "f"; // fill
  }
  if (!graphConfig.errors.empty()) {
    out += "e"; // error bars
  }
  return out;
}

void TrendingMachine::setUserAxesLabels(TAxis* xAxis, TAxis* yAxis, const std::string& graphAxesLabels)
{
  if (std::count(graphAxesLabels.begin(), graphAxesLabels.end(), ':') != 1 && graphAxesLabels != "") {
    ILOG(Error, Support) << "In setup of graphAxesLabels yLabel:xLabel should be divided by one ':'" << ENDM;
    return;
  }
  const std::size_t posDivider = graphAxesLabels.find(':');
  const std::string yLabel(graphAxesLabels.substr(0, posDivider));
  const std::string xLabel(graphAxesLabels.substr(posDivider + 1));

  xAxis->SetTitle(xLabel.data());
  yAxis->SetTitle(yLabel.data());
}

void TrendingMachine::setUserYAxisRange(TH1* hist, const std::string& graphYAxisRange)
{
  if (std::count(graphYAxisRange.begin(), graphYAxisRange.end(), ':') != 1 && graphYAxisRange != "") {
    ILOG(Error, Support) << "In setup of graphYRange yMin:yMax should be divided by one ':'" << ENDM;
    return;
  }
  const std::size_t posDivider = graphYAxisRange.find(':');
  const std::string minString(graphYAxisRange.substr(0, posDivider));
  const std::string maxString(graphYAxisRange.substr(posDivider + 1));

  float yMin = 0.f;
  float yMax = 0.f;
  try {
    yMin = std::stof(minString);
    yMax = std::stof(maxString);
  } catch (const std::logic_error& e) { // invalid_argument or out_of_range
    ILOG(Error, Support) << "Could not parse graphYRange '" << graphYAxisRange << "' as yMin:yMax numbers (" << e.what() << "), ignoring it." << ENDM;
    return;
  }
  if (!(yMin < yMax)) {
    ILOG(Error, Support) << "In setup of graphYRange yMin should be smaller than yMax, got '" << graphYAxisRange << "', ignoring it." << ENDM;
    return;
  }
  hist->GetYaxis()->SetLimits(yMin, yMax);
}

void TrendingMachine::formatTimeXAxis(TH1* background)
{
  background->GetXaxis()->SetTimeDisplay(1);
  // It deals with highly congested dates labels
  background->GetXaxis()->SetNdivisions(505);
  // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
  background->GetXaxis()->SetTimeOffset(0.0);
  background->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
}

void TrendingMachine::formatRunNumberXAxis(TH1* background)
{
  background->GetXaxis()->SetNoExponent(true);
}

bool TrendingMachine::canContinueTrend(TTree* tree, const std::vector<TrendingMachineConfig::DataSource>& dataSources)
{
  if (tree == nullptr) {
    return false;
  }

  size_t expectedNBranches = 1 /* meta */ + 1 /* time */ + dataSources.size();
  if (tree->GetNbranches() != expectedNBranches) {
    ILOG(Warning, Support) << "The retrieved TTree has different number of branches than expected ("
                           << tree->GetNbranches() << " vs. " << expectedNBranches << "). "
                           << "Filling the tree with mismatching branches might produce invalid plots, "
                           << "thus a new tree will be created" << ENDM;
    return false;
  }

  std::set<std::string> expectedBranchNames{ "time", "meta" };
  for (const auto& dataSource : dataSources) {
    expectedBranchNames.insert(dataSource.name);
  }

  std::set<std::string> existingBranchNames;
  for (const auto& branch : *tree->GetListOfBranches()) {
    existingBranchNames.insert(branch->GetName());
  }

  if (expectedBranchNames != existingBranchNames) {
    ILOG(Warning, Support) << "The retrieved TTree has the same number of branches,"
                           << " but at least one has a different name."
                           << " Filling the tree with mismatching branches might produce invalid plots, "
                           << "thus a new tree will be created" << ENDM;
    return false;
  }

  return true;
}
