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
#include "TFile.h"

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void TrendingTaskTPC::configure(std::string name, const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigTPC(name, config);
}

void TrendingTaskTPC::initialize(Trigger, framework::ServiceRegistry&)
{
  // Prepare the data structure of TTree.
  mTrend = std::make_unique<TTree>(); // TODO: retrieve last TTree, so we continue trending. maybe do it optionally?
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("meta", &mMetaData, "runNumber/I");
  mTrend->Branch("time", &mTime);

  // Define the branch for each datasource (TH1, TH2, ...) in the config.
  for (const auto& source : mConfig.dataSources) {    
    std::unique_ptr<ReductorTPC> reductor(root_class_factory::create<ReductorTPC>(source.moduleName, source.reductorName));
    //mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress());
    mReductors[source.name] = std::move(reductor);
  }

  // Initialise the publishing by getting the current mTrend TTree.
  getObjectsManager()->startPublishing(mTrend.get());
}

// Ttodo: see if OptimizeBaskets() indeed helps after some time.
void TrendingTaskTPC::update(Trigger t, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  trendValues(t.timestamp, qcdb);
  generatePlots();
}

void TrendingTaskTPC::finalize(Trigger, framework::ServiceRegistry&)
{
  generatePlots();
}

void TrendingTaskTPC::trendValues(uint64_t timestamp, repository::DatabaseInterface& qcdb)
{
  mTime = timestamp / 1000; // ROOT expects seconds since epoch.
  // Todo get run number when it is available. consider putting it inside monitor object's metadata (this might be not enough if we trend across runs).
  mMetaData.runNumber = -1;

  for (auto& dataSource : mConfig.dataSources) {
    // Todo: make it agnostic to MOs, QOs or other objects. Let the reductor cast to whatever it needs.

    // Get the size of the axisDivision for the number of pads to prepare.
    int axisSize = (int)dataSource.axisDivision.size();
    int innerAxisSize = (int)dataSource.axisDivision[0].size() - 1;
    mNumberPads = axisSize * innerAxisSize;

    if (dataSource.type == "repository") {
      auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, timestamp);
      TObject* obj = mo ? mo->getObject() : nullptr;
      if (obj) {
        ILOG(Info, Support) << "Just before entering the update of the reductor." << ENDM;
        mReductors[dataSource.name]->update(obj, dataSource.axisDivision);
        ILOG(Info, Support) << "Just after the update of the reductor." << ENDM;
    // Get the number of input pads if obj is a TCanvas. Update then the value obtained above.
        if (auto canvas = dynamic_cast<TCanvas*>(obj)) {
          mNumberPads = static_cast<TList*>(canvas->GetListOfPrimitives())->GetEntries();
        }
      }
    } else if (dataSource.type == "repository-quality") {
      if (auto qo = qcdb.retrieveQO(dataSource.path + "/" + dataSource.name, timestamp)) {
        mReductors[dataSource.name]->update(qo.get(), dataSource.axisDivision);
      }
    } else {
      ILOG(Error, Support) << "Unknown type of data source '" << dataSource.type << "'." << ENDM;
    }
  }
  ILOG(Info, Support) << "mTrend will be filled. Interesting no?" << ENDM;
  mTrend->Fill();
  ILOG(Info, Support) << "mTrend has been filled. Interesting no?" << ENDM;
}

void TrendingTaskTPC::generatePlots()
{
  
  TFile *file = TFile::Open("/home/cindy/Desktop/treeTest.root", "RECREATE");
  mTrend->Write();
  file->Close();
  ILOG(Info, Support) << "File saved." << ENDM;
/*
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

    // Draw the raw data on the canvas.
    TCanvas* c = new TCanvas();
    drawCanvas(c, plot.varexp, plot.selection, plot.option, plot.graphErrors, plot.name);

    c->SetName(plot.name.c_str());
    c->SetTitle(plot.title.c_str());

    // Postprocessing the plot - adding specified titles, configuring time-based plots, flushing buffers. Notice that axes and title are drawn using a histogram, even in the case of graphs.
    for (int p = 1; p < mNumberPads + 1; p++) { // Loop over each pad in c.
      c->cd(p);                                 // Go to the corresponding pad.

      if (auto histo = dynamic_cast<TH1*>(c->cd(p)->GetPrimitive("htemp"))) {
         
        // The title of histogram is printed, not the title of canvas => we set it as well.
        //histo->SetTitle(plot.title.c_str());

        // We have to update the canvas to make the title appear.
        //c->Update();

        // After the update, the title has a different size and it is not in the center anymore. We have to fix that.
        if (auto title = dynamic_cast<TPaveText*>(c->GetPrimitive("title"))) {
          title->SetBBoxCenterX(c->GetBBoxCenter().fX);
          // It will have an effect only after invoking Draw again.
          title->Draw();
        } else {
          ILOG(Error, Devel) << "Could not get the title TPaveText of the plot '" << plot.name << "'." << ENDM;
        }
      
        // Is it really needed in our case?

        // We have to explicitly configure showing time on x axis.
        // I hope that looking for ":time" is enough here and someone doesn't come with an exotic use-case.
        if (plot.varexp.find(":time") != std::string::npos) {
          histo->GetXaxis()->SetTimeDisplay(1);
          // It deals with highly congested dates labels
          histo->GetXaxis()->SetNdivisions(505);
          // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
          histo->GetXaxis()->SetTimeOffset(0.0);
          histo->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
        }

        // QCG doesn't empty the buffers before visualizing the plot, nor does ROOT when saving the file, so we have to do it here.
        histo->BufferEmpty();
      } else {
        ILOG(Error, Devel) << "Could not get the htemp histogram of the plot '" << plot.name << "'." << ENDM;
      }
    } // for (int p = 1; p < (mAxisSize*mInnerAxisSize)+1; p++)

    mPlots[plot.name] = c;
    getObjectsManager()->startPublishing(c);
  }*/
}

void TrendingTaskTPC::drawCanvas(TCanvas* thisCanvas, const std::string& var, const std::string& sel, const std::string& opt, const std::string& err, const std::string& name)
{
  // We determine the order of the plot, i.e. if it is a histogram (1), graph (2), or any higher dimension.
  const size_t plotOrder = std::count(var.begin(), var.end(), ':') + 1;

  // We have to delete the graph errors after the plot is saved, unfortunately the canvas does not take its ownership.
  TGraphErrors* graphErrors = nullptr;

  // Divide the canvas into pads according to the type of plot.
  thisCanvas->DivideSquare(mNumberPads);

  for (int p = 0; p < (mNumberPads); p++) {
    thisCanvas->cd(p + 1);                 // Go to the corresponding pad, starting from 1.
    std::size_t posEndVar = var.find("."); // Find the end of the y-variable.
    //std::string varPad(var.substr(0, posEndVar) + "[" + p + "]" + var.substr(posEndVar));
    std::string varPad(var.substr(0, posEndVar) + ".at(" + p + ")" + var.substr(posEndVar));
    ILOG(Info, Support) << "varPad " << varPad << ENDM;

    mTrend->Draw(varPad.c_str(), sel.c_str(), opt.c_str());

    // For graphs we allow to draw errors if they are specified.
    if (!err.empty()) {
      if (plotOrder != 2) {
        ILOG(Error, Support) << "Non empty graphErrors seen for the plot '" << name << "', which is not a graph, ignoring." << ENDM;
      } else {
        // We generate some 4-D points, where 2 dimensions represent graph points and 2 others are the error bars. The errors are given as errX:errY.
        std::string varexpWithErrors(varPad + ":" + err + "[" + p + "]");
        mTrend->Draw(varexpWithErrors.c_str(), sel.c_str(), "goff");
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

} // namespace o2::quality_control_modules::tpc
