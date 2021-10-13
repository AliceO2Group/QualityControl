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
/// \file   TrendingTaskTPC.cxx
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///

#include "TPC/TrendingTaskTPC.h"
#include "TPC/ReductorTPC.h"
#include "TPC/TH1ReductorTPC.h"
#include "TPC/TH2ReductorTPC.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/RootClassFactory.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TDatime.h>
#include <TGraphErrors.h>
#include <TPoint.h>
#include <TFile.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::tpc;

void TrendingTaskTPC::configure(std::string name,
                                const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigTPC(name, config);
}

void TrendingTaskTPC::initialize(Trigger, framework::ServiceRegistry&)
{
  // Prepare the data structure of TTree.
  mTrend = std::make_unique<TTree>();   // TODO: retrieve last TTree, so we
                                        // continue trending. Maybe do it optionally?
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("meta", &mMetaData, "runNumber/I");
  mTrend->Branch("time", &mTime);

  for (const auto& source : mConfig.dataSources) {
    mSources[source.name] = std::vector<SliceInfo>();

    std::unique_ptr<ReductorTPC> reductor(root_class_factory::create<ReductorTPC>(
      source.moduleName, source.reductorName));
    mTrend->Branch(source.name.c_str(), &mSources[source.name]);
    mReductors[source.name] = std::move(reductor);
  }

  // Initialise the publishing with the current TTree.
  getObjectsManager()->startPublishing(mTrend.get());
}

// TODO: see if OptimizeBaskets() indeed helps after some time.
void TrendingTaskTPC::update(Trigger t, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  trendValues(t.timestamp, qcdb);

  TFile *file = TFile::Open("/home/cindy/Desktop/treeTest.root", "RECREATE");
  mTrend->Write();
  file->Close();
  ILOG(Info, Support) << "File saved." << ENDM;

  //generatePlots();
}

void TrendingTaskTPC::finalize(Trigger, framework::ServiceRegistry&)
{
  generatePlots();
}

void TrendingTaskTPC::trendValues(uint64_t timestamp,
                                  repository::DatabaseInterface& qcdb)
{
  mTime = timestamp/1000; // ROOT expects seconds since epoch.
  mMetaData.runNumber = -1;

  for (auto& dataSource : mConfig.dataSources) {
    if (dataSource.type == "repository") {
      auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, timestamp);
      TObject* obj = mo ? mo->getObject() : nullptr;
      if (obj) {
        mReductors[dataSource.name]->update(obj, mSources[dataSource.name], dataSource.axisDivision);
        if (auto canvas = dynamic_cast<TCanvas*>(obj)) {  // Case we have an input canvas. 
          mNumberPads = static_cast<TList*>(canvas->GetListOfPrimitives())->GetEntries();
        } else if ((int)dataSource.axisDivision[0].size() > 1) {  // Case we have a sliced single histogram.
          int axisSize = (int)dataSource.axisDivision.size();
          int innerAxisSize = (int)dataSource.axisDivision[0].size() - 1;
          mNumberPads = axisSize * innerAxisSize;
        } else {
          mNumberPads = 1;
        }
      }
    } else if (dataSource.type == "repository-quality") {
      if (auto qo = qcdb.retrieveQO(
          dataSource.path + "/" + dataSource.name, timestamp)) {
        mReductors[dataSource.name]->update(qo.get(), mSources[dataSource.name],
          dataSource.axisDivision);
      }
    } else {
      ILOG(Error, Support) << "Data source '" << dataSource.type << "' unknown." << ENDM;
    }
  }

  mTrend->Fill();
  //for (auto& dataSource : mConfig.dataSources) {
    //mSources[dataSource.name].clear();
  //}
  ILOG(Info, Support) << "mTrend has been filled." << ENDM;
}

void TrendingTaskTPC::generatePlots()
{
  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, no plot." << ENDM;
    return;
  }

  ILOG(Info, Support) << "Generating " << mConfig.plots.size() << " plots." << ENDM;
  for (const auto& plot : mConfig.plots) {
    // Delete the existing plots before regenerating them.
    if (mPlots.count(plot.name)) {
      getObjectsManager()->stopPublishing(plot.name);
      delete mPlots[plot.name];
    }

    // Draw the trending on a new canvas.
    TCanvas* c = new TCanvas();
    drawCanvas(c, plot.varexp, plot.selection, plot.option, plot.graphErrors, plot.name);
    ILOG(Info, Support) << "Canvas has been drawn for " << plot.varexp << ENDM;

    c->SetName(plot.name.c_str());
    c->SetTitle(plot.title.c_str());

    // Postprocessing the plots (titles, axes, flushing buffers) via an histogram.
    for (int p = 1; p < mNumberPads + 1; p++) { // Loop over each pad in c.
      c->cd(p);

      if (auto histo = dynamic_cast<TH1*>(c->cd(p)->GetPrimitive("htemp"))) {
        // Set a centered, nicely formatted title.
        if (auto title = dynamic_cast<TPaveText*>(c->GetPrimitive("title"))) {
          title->SetBBoxCenterX(c->GetBBoxCenter().fX);
          title->Draw();
        } else {
          ILOG(Error, Devel) << "Could not get the title 'TPaveText' of the plot '"
            << plot.name << "'." << ENDM;
        }

        // Configure the time for the x axis.
        if (plot.varexp.find(":time") != std::string::npos) {
          histo->GetXaxis()->SetTimeDisplay(1);
          histo->GetXaxis()->SetNdivisions(505);
          histo->GetXaxis()->SetTimeOffset(0.0);
          histo->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
        }

        // Manually empty the buffers before visualising the plot.
        histo->BufferEmpty();
      } else {
        ILOG(Error, Devel) << "Could not get the htemp histogram of the plot '" << plot.name << "'." << ENDM;
      }
    }

    mPlots[plot.name] = c;
    getObjectsManager()->startPublishing(c);
  }

  for (auto& dataSource : mConfig.dataSources) {
    mSources[dataSource.name].clear();
  }
}

void TrendingTaskTPC::drawCanvas(TCanvas* thisCanvas, const std::string& var,
  const std::string& sel, const std::string& opt, const std::string& err, const std::string& name)
{
  // Divide the canvas in the correct number of pads.
  thisCanvas->DivideSquare(mNumberPads);

  // Determine the order of the plot (1 - histo, 2 - graph, ...)
  const size_t plotOrder = std::count(var.begin(), var.end(), ':') + 1;

  // Delete the graph errors after the plot is saved.
  // Unfortunately the canvas does not take its ownership.
  TGraphErrors* graphErrors = nullptr;

  for (int p = 0; p < (mNumberPads); p++) {
    thisCanvas->cd(p + 1);
    //std::size_t posEndVar = var.find(".");  // Find the end of the y-variable.
    //std::string varPad(var.substr(0, posEndVar) + "[" + p + "]" + var.substr(posEndVar));
    //std::string varPad(var.substr(0, posEndVar) + ".at(" + p + ")" + var.substr(posEndVar));
    //ILOG(Info, Support) << "varPad " << varPad << ENDM;

    mTrend->Draw(var.c_str(), sel.c_str(), opt.c_str());  ///// varpad

    // For graphs, we allow to draw errors if they are specified.
    if (!err.empty()) {
      if (plotOrder != 2) {
        ILOG(Error, Support) << "Non empty graphErrors seen for the plot '" << name << "', which is not a graph, ignoring." << ENDM;
      } else {
        // We generate some 4-D points, where 2 dimensions represent graph points and 2 others are the error bars. The errors are given as errX:errY.
        //std::string varexpWithErrors(varPad + ":" + err + "[" + p + "]");
        mTrend->Draw(var.c_str(), sel.c_str(), "goff");
        graphErrors = new TGraphErrors(mTrend->GetSelectedRows(), mTrend->GetVal(1), mTrend->GetVal(0), mTrend->GetVal(2), mTrend->GetVal(3));

        // We draw on the same plot as the main graph, but only error bars.
        graphErrors->Draw("SAME E");

        // We try to convince ROOT to delete graphErrors together with the rest of the canvas.
        if (auto* pad = thisCanvas->GetPad(p + 1)) {
          if (auto* primitives = pad->GetListOfPrimitives()) {
            primitives->Add(graphErrors);
          }
        }
      }
    } // if (!err.empty())
  }   // for (int p = 0; p < (mAxisSize*mInnerAxisSize); p++)
}