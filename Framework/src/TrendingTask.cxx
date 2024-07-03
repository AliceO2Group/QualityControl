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
/// \file    TrendingTask.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/TrendingTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ReductorHelpers.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/ActivityHelpers.h"

#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TGraphErrors.h>
#include <TPoint.h>
#include <TStyle.h>
#include <TLegend.h>

#include <boost/algorithm/string.hpp>
#include <set>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

void TrendingTask::configure(const boost::property_tree::ptree& config)
{
  // we clear any existing objects, which would be there only in case of reconfiguration
  // at the time of writing, this not even supported by ECS
  mReductors.clear();
  mTrend.reset();

  // configuration
  mConfig = TrendingTaskConfig(getID(), config);
  for (const auto& source : mConfig.dataSources) {
    mReductors.emplace(source.name, root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
  }
}

bool TrendingTask::canContinueTrend(TTree* tree)
{
  if (tree == nullptr) {
    return false;
  }

  size_t expectedNBranches = 1 /* meta */ + 1 /* time */ + mConfig.dataSources.size();
  if (tree->GetNbranches() != expectedNBranches) {
    ILOG(Warning, Support) << "The retrieved TTree has different number of branches than expected ("
                           << tree->GetNbranches() << " vs. " << expectedNBranches << "). "
                           << "Filling the tree with mismatching branches might produce invalid plots, "
                           << "thus a new tree will be created" << ENDM;
    return false;
  }

  std::set<std::string> expectedBranchNames{ "time", "meta" };
  for (const auto& dataSource : mConfig.dataSources) {
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

void TrendingTask::initializeTrend(o2::quality_control::repository::DatabaseInterface& qcdb)
{
  // tree exists and we can reuse it
  if (canContinueTrend(mTrend.get())) {
    if (mConfig.resumeTrend == false) {
      mTrend->Reset();
    } else {
      ILOG(Info, Support) << "Will continue the trend from the previous run." << ENDM;
    }
    return;
  }

  // tree is not reusable or does not exist => if we want to reuse the latest, we look for it in QCDB
  mTrend.reset();
  if (mConfig.resumeTrend) {
    ILOG(Info, Support) << "Trying to retrieve an existing TTree for this task to continue the trend." << ENDM;
    auto path = RepoPathUtils::getMoPath(mConfig.detectorName, PostProcessingInterface::getName(), "", "", false);
    auto mo = qcdb.retrieveMO(path, PostProcessingInterface::getName(), repository::DatabaseInterface::Timestamp::Latest);
    if (mo && mo->getObject()) {
      auto tree = dynamic_cast<TTree*>(mo->getObject());
      if (tree) {
        mTrend = std::unique_ptr<TTree>(tree);
        mo->setIsOwner(false);
      }
    } else {
      ILOG(Warning, Support) << "Could not retrieve an existing TTree for this task" << ENDM;
    }
    if (canContinueTrend(mTrend.get())) {
      mTrend->SetBranchAddress("meta", &mMetaData);
      mTrend->SetBranchAddress("time", &mTime);
      for (const auto& [sourceName, reductor] : mReductors) {
        mTrend->SetBranchAddress(sourceName.c_str(), reductor->getBranchAddress());
      }
      ILOG(Info, Support) << "Will use the latest TTree from QCDB for this task to continue the trend." << ENDM;
      return;
    } else {
      mTrend.reset();
    }
  }

  // we could not reuse the tree or never had one => we create a new one
  if (mTrend == nullptr) {
    mTrend = std::make_unique<TTree>();
    mTrend->SetName(PostProcessingInterface::getName().c_str());

    mTrend->Branch("meta", &mMetaData, mMetaData.getBranchLeafList());
    mTrend->Branch("time", &mTime);
    for (const auto& [sourceName, reductor] : mReductors) {
      mTrend->Branch(sourceName.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    }
  }
}

void TrendingTask::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // removing leftovers from any previous runs
  mPlots.clear();

  initializeTrend(services.get<repository::DatabaseInterface>());

  if (mConfig.producePlotsOnUpdate) {
    getObjectsManager()->startPublishing(mTrend.get(), PublicationPolicy::ThroughStop);
  }
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TrendingTask::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);
  if (mConfig.producePlotsOnUpdate) {
    generatePlots();
  }
}

void TrendingTask::finalize(Trigger, framework::ServiceRegistryRef)
{
  if (!mConfig.producePlotsOnUpdate) {
    getObjectsManager()->startPublishing(mTrend.get());
  }
  generatePlots();
}

void TrendingTask::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  mTime = activity_helpers::isLegacyValidity(t.activity.mValidity)
            ? t.timestamp / 1000
            : t.activity.mValidity.getMax() / 1000; // ROOT expects seconds since epoch.
  mMetaData.runNumber = t.activity.mId;

  for (auto& dataSource : mConfig.dataSources) {
    if (!reductor_helpers::updateReductor(mReductors[dataSource.name].get(), t, dataSource, qcdb, *this)) {
      ILOG(Error, Support) << "Failed to update reductor for data sources with path '" << dataSource.path
                           << "', name '" << dataSource.name
                           << "', type '" << dataSource.type << "'." << ENDM;
    }
  }

  mTrend->Fill();
}

void TrendingTask::setUserAxesLabels(TAxis* xAxis, TAxis* yAxis, const std::string& graphAxesLabels)
{
  // todo if we keep adding this method to pp classes we should move it up somewhere
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

void TrendingTask::setUserYAxisRange(TH1* hist, const std::string& graphYAxisRange)
{
  if (std::count(graphYAxisRange.begin(), graphYAxisRange.end(), ':') != 1 && graphYAxisRange != "") {
    ILOG(Error, Support) << "In setup of graphYRange yMin:yMax should be divided by one ':'" << ENDM;
    return;
  }
  const std::size_t posDivider = graphYAxisRange.find(':');
  const std::string minString(graphYAxisRange.substr(0, posDivider));
  const std::string maxString(graphYAxisRange.substr(posDivider + 1));

  const float yMin = std::stof(minString);
  const float yMax = std::stof(maxString);
  hist->GetYaxis()->SetLimits(yMin, yMax);
}

void TrendingTask::formatRunNumberXAxis(TH1* background)
{
  background->GetXaxis()->SetNoExponent(true);
}

void TrendingTask::formatTimeXAxis(TH1* background)
{
  background->GetXaxis()->SetTimeDisplay(1);
  // It deals with highly congested dates labels
  background->GetXaxis()->SetNdivisions(505);
  // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
  background->GetXaxis()->SetTimeOffset(0.0);
  background->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
}

void TrendingTask::generatePlots()
{
  if (mTrend == nullptr) {
    ILOG(Info, Support) << "The trend object is not there, won't generate any plots." << ENDM;
    return;
  }

  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, won't generate any plots." << ENDM;
    return;
  }

  ILOG(Info, Support) << "Generating " << mConfig.plots.size() << " plots." << ENDM;
  for (const auto& plotConfig : mConfig.plots) {

    // Before we generate any new plots, we have to delete existing under the same names.
    // It seems that ROOT cannot handle an existence of two canvases with a common name in the same process.
    if (mPlots.count(plotConfig.name)) {
      mPlots[plotConfig.name].reset();
    }
    auto c = drawPlot(plotConfig);
    mPlots[plotConfig.name].reset(c);
    getObjectsManager()->startPublishing(c, PublicationPolicy::Once);
  }
}

std::string TrendingTask::deduceGraphLegendOptions(const TrendingTaskConfig::Graph& graphConfig)
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

TCanvas* TrendingTask::drawPlot(const TrendingTaskConfig::Plot& plotConfig)
{
  auto* c = new TCanvas();
  auto* legend = new TLegend(0.3, 0.2);

  if (plotConfig.colorPalette != 0) {
    // this will work just once until we bump ROOT to a version which contains this commit:
    // https://github.com/root-project/root/commit/0acdbd5be80494cec98ff60ba9a73cfe70a9a57a
    // and enable the commented out line
    // perhaps JSROOT >7.7.1 will allow us to retain the palette as well.
    gStyle->SetPalette(plotConfig.colorPalette);
    // This makes ROOT store the selected palette for each generated plot.
    // TColor::DefinedColors(1); // TODO enable when available
  } else {
    // we set the default palette
    gStyle->SetPalette();
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

    mTrend->Draw(graphConfig.varexp.c_str(), graphConfig.selection.c_str(), option.c_str());

    // For graphs, we allow to draw errors if they are specified.
    TGraphErrors* graphErrors = nullptr;
    if (!graphConfig.errors.empty()) {
      if (plotOrder != 2) {
        ILOG(Error, Support) << "Non empty graphErrors seen for the plotConfig '" << plotConfig.name << "', which is not a graphConfig, ignoring." << ENDM;
      } else {
        // We generate some 4-D points, where 2 dimensions represent graph points and 2 others are the error bars
        std::string varexpWithErrors(graphConfig.varexp + ":" + graphConfig.errors);
        mTrend->Draw(varexpWithErrors.c_str(), graphConfig.selection.c_str(), "goff");
        graphErrors = new TGraphErrors(mTrend->GetSelectedRows(), mTrend->GetVal(1), mTrend->GetVal(0),
                                       mTrend->GetVal(2), mTrend->GetVal(3));
        graphErrors->SetName((graphConfig.name + "_errors").c_str());
        graphErrors->SetTitle((graphConfig.title + " errors").c_str());
        // We draw on the same plotConfig as the main graphConfig, but only error bars
        graphErrors->Draw("SAME E");
      }
    }

    if (auto graph = dynamic_cast<TGraph*>(c->FindObject("Graph"))) {
      graph->SetName(graphConfig.name.c_str());
      graph->SetTitle(graphConfig.title.c_str());
      legend->AddEntry(graph, graphConfig.title.c_str(), deduceGraphLegendOptions(graphConfig).c_str());
    }
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
      if (background == nullptr) {
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
