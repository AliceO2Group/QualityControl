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
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/ActivityHelpers.h"

#include <TH1.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TGraphErrors.h>
#include <TPoint.h>

#include <set>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

void TrendingTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfig(getID(), config);
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

void TrendingTask::initialize(Trigger, framework::ServiceRegistryRef services)
{
  // Preparing data structure of TTree
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
      ILOG(Warning, Support)
        << "Could not retrieve an existing TTree for this task, maybe there is none which match these Activity settings"
        << ENDM;
    }
  }
  for (const auto& source : mConfig.dataSources) {
    mReductors.emplace(source.name, root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
  }

  if (mTrend == nullptr || !canContinueTrend(mTrend.get())) {
    mTrend = std::make_unique<TTree>();
    mTrend->SetName(PostProcessingInterface::getName().c_str());

    mTrend->Branch("meta", &mMetaData, mMetaData.getBranchLeafList());
    mTrend->Branch("time", &mTime);
    for (const auto& [sourceName, reductor] : mReductors) {
      mTrend->Branch(sourceName.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    }
  } else {
    mTrend->SetBranchAddress("meta", &mMetaData);
    mTrend->SetBranchAddress("time", &mTime);
    for (const auto& [sourceName, reductor] : mReductors) {
      mTrend->SetBranchAddress(sourceName.c_str(), reductor->getBranchAddress());
    }
  }
  if (mConfig.producePlotsOnUpdate) {
    getObjectsManager()->startPublishing(mTrend.get());
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

    // todo: make it agnostic to MOs, QOs or other objects. Let the reductor cast to whatever it needs.
    if (dataSource.type == "repository") {
      auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
      TObject* obj = mo ? mo->getObject() : nullptr;
      if (obj) {
        mReductors[dataSource.name]->update(obj);
      }
    } else if (dataSource.type == "repository-quality") {
      auto qo = qcdb.retrieveQO(dataSource.path + "/" + dataSource.name, t.timestamp, t.activity);
      if (qo) {
        mReductors[dataSource.name]->update(qo.get());
      }
    } else {
      ILOG(Error, Support) << "Unknown type of data source '" << dataSource.type << "'." << ENDM;
    }
  }

  mTrend->Fill();
}

void TrendingTask::setUserAxisLabel(TAxis* xAxis, TAxis* yAxis, const std::string& graphAxisLabel)
{
  // todo if we keep adding this method to pp classes we should move it up somewhere
  if (std::count(graphAxisLabel.begin(), graphAxisLabel.end(), ':') != 1 && graphAxisLabel != "") {
    ILOG(Error, Support) << "In setup of graphAxisLabel yLabel:xLabel should be divided by one ':'" << ENDM;
    return;
  }
  const std::size_t posDivider = graphAxisLabel.find(':');
  const std::string yLabel(graphAxisLabel.substr(0, posDivider));
  const std::string xLabel(graphAxisLabel.substr(posDivider + 1));

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

  for (const auto& plot : mConfig.plots) {

    // Before we generate any new plots, we have to delete existing under the same names.
    // It seems that ROOT cannot handle an existence of two canvases with a common name in the same process.
    if (mPlots.count(plot.name)) {
      getObjectsManager()->stopPublishing(plot.name);
      delete mPlots[plot.name];
    }

    // we determine the order of the plot, i.e. if it is a histogram (1), graph (2), or any higher dimension.
    const size_t plotOrder = std::count(plot.varexp.begin(), plot.varexp.end(), ':') + 1;
    // we have to delete the graph errors after the plot is saved, unfortunately the canvas does not take its ownership
    TGraphErrors* graphErrors = nullptr;

    auto* c = new TCanvas();

    mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), plot.option.c_str());

    c->SetName(plot.name.c_str());
    c->SetTitle(plot.title.c_str());

    // For graphs we allow to draw errors if they are specified.
    if (!plot.graphErrors.empty()) {
      if (plotOrder != 2) {
        ILOG(Error, Support) << "Non empty graphErrors seen for the plot '" << plot.name << "', which is not a graph, ignoring." << ENDM;
      } else {
        // We generate some 4-D points, where 2 dimensions represent graph points and 2 others are the error bars
        std::string varexpWithErrors(plot.varexp + ":" + plot.graphErrors);
        mTrend->Draw(varexpWithErrors.c_str(), plot.selection.c_str(), "goff");
        graphErrors = new TGraphErrors(mTrend->GetSelectedRows(), mTrend->GetVal(1), mTrend->GetVal(0), mTrend->GetVal(2), mTrend->GetVal(3));
        // We draw on the same plot as the main graph, but only error bars
        graphErrors->Draw("SAME E");
      }
    }

    if (!plot.graphAxisLabel.empty()) {
      if (auto histo = dynamic_cast<TH2F*>(c->GetPrimitive("htemp"))) {
        setUserAxisLabel(histo->GetXaxis(), histo->GetYaxis(), plot.graphAxisLabel);
      }
    }

    // Postprocessing the plot - adding specified titles, configuring time-based plots, flushing buffers.
    // Notice that axes and title are drawn using a histogram, even in the case of graphs.
    if (auto histo = dynamic_cast<TH1*>(c->GetPrimitive("htemp"))) {
      // The title of histogram is printed, not the title of canvas => we set it as well.
      histo->SetTitle(plot.title.c_str());
      // We have to update the canvas to make the title appear.
      c->Update();

      // After the update, the title has a different size and it is not in the center anymore. We have to fix that.
      if (auto title = dynamic_cast<TPaveText*>(c->GetPrimitive("title"))) {
        title->SetBBoxCenterX(c->GetBBoxCenter().fX);
        c->Modified();
        c->Update();
      } else {
        ILOG(Error, Devel) << "Could not get the title TPaveText of the plot '" << plot.name << "'." << ENDM;
      }

      // We have to explicitly configure showing time on x axis.
      // I hope that looking for ":time" is enough here and someone doesn't come with an exotic use-case.
      if (plot.varexp.find(":time") != std::string::npos) {
        histo->GetXaxis()->SetTimeDisplay(1);
        // It deals with highly congested dates labels
        histo->GetXaxis()->SetNdivisions(505);
        // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
        histo->GetXaxis()->SetTimeOffset(0.0);
        histo->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
      } else if (plot.varexp.find(":meta.runNumber") != std::string::npos) {
        histo->GetXaxis()->SetNoExponent(true);
      }

      // Set the user-defined range on the y axis if needed.
      if (!plot.graphYRange.empty()) {
        setUserYAxisRange(histo, plot.graphYRange);
        c->Modified();
        c->Update();
      }

      // QCG doesn't empty the buffers before visualizing the plot, nor does ROOT when saving the file,
      // so we have to do it here.
      histo->BufferEmpty();
    } else {
      ILOG(Error, Devel) << "Could not get the htemp histogram of the plot '" << plot.name << "'." << ENDM;
    }

    mPlots[plot.name] = c;
    getObjectsManager()->startPublishing(c);
  }
}
