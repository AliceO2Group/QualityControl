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
    auto&& [emplaced, _] = mReductors.emplace(source.name, root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
    emplaced->second->setCustomConfig(source.reductorParameters);
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

static inline bool hasAnyStyle(const TrendingTaskConfig::GraphStyle& s) {
  return s.lineColor >= 0 || s.lineStyle >= 0 || s.lineWidth >= 0 ||
         s.markerColor >= 0 || s.markerStyle >= 0 || s.markerSize > 0.f ||
         s.fillColor >= 0 || s.fillStyle >= 0;
}

template<class T> // TGraph*, TH1*, etc. (anything with TAttLine/TAttMarker/TAttFill)
static inline void applyStyleIfAny(T* obj, const TrendingTaskConfig::GraphStyle& s) {
  if (!hasAnyStyle(s) || !obj) return;

  // Colors
  if (s.lineColor >= 0)     obj->SetLineColor(s.lineColor);
  if (s.markerColor >= 0)   obj->SetMarkerColor(s.markerColor);

  if (s.lineStyle >= 0)   obj->SetLineStyle(s.lineStyle);   // TAttLine
  if (s.lineWidth >= 0)   obj->SetLineWidth(s.lineWidth);   // TAttLine
  if (s.markerStyle >= 0) obj->SetMarkerStyle(s.markerStyle); // TAttMarker
  if (s.markerSize > 0.f) obj->SetMarkerSize(s.markerSize);   // TAttMarker
  if (s.fillColor >= 0)   obj->SetFillColor(s.fillColor);     // TAttFill
  if (s.fillStyle >= 0)   obj->SetFillStyle(s.fillStyle);     // TAttFill
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

  const auto allSourcesInvoked = trendValues(t, qcdb);
  if (mConfig.producePlotsOnUpdate && (!mConfig.trendIfAllInputs || allSourcesInvoked)) {
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

bool TrendingTask::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  if (mConfig.trendingTimestamp == "trigger") {
    // ROOT expects seconds since epoch.
    mTime = t.timestamp / 1000;
  } else if (mConfig.trendingTimestamp == "validFrom") {
    mTime = t.activity.mValidity.getMin() / 1000;
  } else { // validUntil
    mTime = t.activity.mValidity.getMax() / 1000;
  }
  mMetaData.runNumber = t.activity.mId;
  std::snprintf(mMetaData.runNumberStr, MaxRunNumberStringLength + 1, "%d", t.activity.mId);
  bool wereAllSourcesInvoked = true;

  for (auto& dataSource : mConfig.dataSources) {
    if (!reductor_helpers::updateReductor(mReductors[dataSource.name].get(), t, dataSource, qcdb, *this)) {
      wereAllSourcesInvoked = false;
      ILOG(Error, Support) << "Failed to update reductor for data sources with path '" << dataSource.path
                           << "', name '" << dataSource.name
                           << "', type '" << dataSource.type << "'." << ENDM;
    }
  }

  if (!mConfig.trendIfAllInputs || wereAllSourcesInvoked) {
    mTrend->Fill();
  }

  return wereAllSourcesInvoked;
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

  // Legend (NDC coordinates if enabled in config)
  TLegend* legend = nullptr;
  if (plotConfig.legend.enabled) {
    legend = new TLegend(plotConfig.legend.x1, plotConfig.legend.y1,
                         plotConfig.legend.x2, plotConfig.legend.y2,
                         /*header=*/nullptr, /*option=*/"NDC");
    if (plotConfig.legend.nColumns > 0) {
      legend->SetNColumns(plotConfig.legend.nColumns);
    }
  } else {
    legend = new TLegend(0.30, 0.20, 0.55, 0.35, nullptr, "NDC");
  }
  legend->SetBorderSize(0);
  legend->SetFillStyle(0);
  legend->SetTextSize(0.03);
  legend->SetMargin(0.15);

  // Keep palette behavior unless user forces explicit colors via per-graph style
  if (plotConfig.colorPalette != 0) {
    gStyle->SetPalette(plotConfig.colorPalette);
  } else {
    gStyle->SetPalette(); // default
  }

  // Helpers
  auto resolveColor = [](int idx) -> Color_t {
    if (idx >= 0) {
      return static_cast<Color_t>(idx);
    }
    return static_cast<Color_t>(-1);
  };

  auto getLastDrawnGraph = []() -> TGraph* {
    if (!gPad) return nullptr;
    TGraph* last = nullptr;
    TIter it(gPad->GetListOfPrimitives());
    while (TObject* obj = it()) {
      if (obj->InheritsFrom(TGraph::Class())) {
        last = static_cast<TGraph*>(obj);
      }
    }
    return last;
  };

  auto applyStyleToGraph = [&](TGraph* gr, const TrendingTaskConfig::GraphStyle& st) {
    if (!gr) return;
    const Color_t ln = resolveColor(st.lineColor);
    const Color_t mk = resolveColor(st.markerColor);
    const Color_t fl = resolveColor(st.fillColor);

    if (ln >= 0) gr->SetLineColor(ln);
    if (st.lineStyle >= 0) gr->SetLineStyle(st.lineStyle);
    if (st.lineWidth >= 0) gr->SetLineWidth(st.lineWidth);

    if (mk >= 0) gr->SetMarkerColor(mk);
    if (st.markerStyle >= 0) gr->SetMarkerStyle(st.markerStyle);
    if (st.markerSize >= 0.f) gr->SetMarkerSize(st.markerSize);

    if (fl >= 0) gr->SetFillColor(fl);
    if (st.fillStyle >= 0) gr->SetFillStyle(st.fillStyle);
  };

  // Regardless of drawing kind, TTree::Draw produces a TH1 "htemp" used for axes/title
  TH1* background = nullptr;
  bool firstGraphInPlot = true;

  for (const auto& graphConfig : plotConfig.graphs) {
    // plotOrder: 1 -> histogram; 2 -> graph; >=3 -> multi-dim
    const size_t plotOrder = std::count(graphConfig.varexp.begin(), graphConfig.varexp.end(), ':') + 1;

    // Add "SAME" from the second draw on this canvas
    std::string option = firstGraphInPlot ? graphConfig.option : "SAME " + graphConfig.option;

    // Draw main series
    mTrend->Draw(graphConfig.varexp.c_str(), graphConfig.selection.c_str(), option.c_str());

    // Optionally draw errors (xerr, yerr) as TGraphErrors on top
    TGraphErrors* graphErrors = nullptr;
    if (!graphConfig.errors.empty()) {
      if (plotOrder == 2) {
        std::string varexpWithErrors(graphConfig.varexp + ":" + graphConfig.errors);
        mTrend->Draw(varexpWithErrors.c_str(), graphConfig.selection.c_str(), "goff");
        graphErrors = new TGraphErrors(mTrend->GetSelectedRows(),
                                       mTrend->GetVal(1), mTrend->GetVal(0),
                                       mTrend->GetVal(2), mTrend->GetVal(3));
        graphErrors->SetName((graphConfig.name + "_errors").c_str());
        graphErrors->SetTitle((graphConfig.title + " errors").c_str());
        graphErrors->Draw("SAME E");
      } else {
        ILOG(Error, Support) << "Non-empty 'errors' for plot '" << plotConfig.name
                             << "' but varexp is not a 2D graph; ignoring errors." << ENDM;
      }
    }

    // Style objects after Draw so we override palette/auto styling when requested
    if (plotOrder >= 2) {
      if (auto* gr = getLastDrawnGraph()) {
        applyStyleToGraph(gr, graphConfig.style);
        // Keep errors visually consistent with the main series
        if (graphErrors) {
          applyStyleToGraph(graphErrors, graphConfig.style);
        }
      }
    }

    // Handle axes/title carrier histogram
    if (auto* htemp = dynamic_cast<TH1*>(c->FindObject("htemp"))) {
      if (plotOrder == 1) {
        htemp->SetName(graphConfig.name.c_str());
        htemp->SetTitle(graphConfig.title.c_str());
        legend->AddEntry(htemp, graphConfig.title.c_str(), "lpf");
      } else {
        htemp->SetName("background");
        htemp->SetTitle("background");
      }
      htemp->BufferEmpty();
      if (!background) background = htemp;
    }

    // Legend entry for graphs
    if (auto* gr = dynamic_cast<TGraph*>(c->FindObject("Graph"))) {
      gr->SetName(graphConfig.name.c_str());
      gr->SetTitle(graphConfig.title.c_str());
      legend->AddEntry(gr, graphConfig.title.c_str(),
                       deduceGraphLegendOptions(graphConfig).c_str());
    }

    firstGraphInPlot = false;
  }

  c->SetName(plotConfig.name.c_str());
  c->SetTitle(plotConfig.title.c_str());

  // Post-process: title, axes labels, time axis formatting, y-range
  if (background) {
    background->SetTitle(plotConfig.title.c_str());
    c->Update();

    if (auto* title = dynamic_cast<TPaveText*>(c->GetPrimitive("title"))) {
      title->SetBBoxCenterX(c->GetBBoxCenter().fX);
      c->Modified();
      c->Update();
    } else {
      ILOG(Error, Devel) << "Could not get TPaveText for title of '" << plotConfig.name << "'." << ENDM;
    }

    if (!plotConfig.graphAxisLabel.empty()) {
      setUserAxesLabels(background->GetXaxis(), background->GetYaxis(), plotConfig.graphAxisLabel);
    }

    if (!plotConfig.graphs.empty()) {
      const auto& lastVar = plotConfig.graphs.back().varexp;
      if (lastVar.find(":time") != std::string::npos) {
        formatTimeXAxis(background);
      } else if (lastVar.find(":meta.runNumber") != std::string::npos) {
        formatRunNumberXAxis(background);
      }
    }

    if (!plotConfig.graphYRange.empty()) {
      setUserYAxisRange(background, plotConfig.graphYRange);
      c->Modified();
      c->Update();
    }
  } else {
    ILOG(Error, Devel) << "Could not get 'htemp' for plot '" << plotConfig.name << "'." << ENDM;
  }

  if (plotConfig.graphs.size() > 1 || plotConfig.legend.enabled) {
    legend->Draw();
  } else {
    delete legend;
  }

  c->Modified();
  c->Update();
  return c;
}
