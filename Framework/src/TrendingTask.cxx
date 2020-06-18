// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
#include "RootClassFactory.h"
#include <boost/property_tree/ptree.hpp>
#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;

void TrendingTask::configure(std::string name, const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfig(name, config);
}

void TrendingTask::initialize(Trigger, framework::ServiceRegistry& services)
{
  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>(); // todo: retrieve last TTree, so we continue trending. maybe do it optionally?
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("meta", &mMetaData, "runNumber/I");
  mTrend->Branch("time", &mTime);

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<Reductor> reductor(root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }

  // Setting up services
  mDatabase = &services.get<repository::DatabaseInterface>();
}

//todo: see if OptimizeBaskets() indeed helps after some time
void TrendingTask::update(Trigger, framework::ServiceRegistry&)
{
  trendValues();

  storePlots();
  storeTrend();
}

void TrendingTask::finalize(Trigger, framework::ServiceRegistry&)
{
  storePlots();
  storeTrend();
}

void TrendingTask::storeTrend()
{
  ILOG(Info) << "Storing the trend, entries: " << mTrend->GetEntries() << ENDM;

  auto mo = std::make_shared<core::MonitorObject>(mTrend.get(), getName(), mConfig.detectorName);
  mo->setIsOwner(false);
  mDatabase->storeMO(mo);
}

void TrendingTask::trendValues()
{
  // We use current date and time. This for planned processing (not history). We still might need to use the objects
  // timestamps in the end, but this would become ambiguous if there is more than one data source.
  mTime = TDatime().Convert();
  // todo get run number when it is available. consider putting it inside monitor object's metadata (this might be not
  //  enough if we trend across runs).
  mMetaData.runNumber = -1;

  for (auto& dataSource : mConfig.dataSources) {

    // todo: make it agnostic to MOs, QOs or other objects. Let the reductor cast to whatever it needs.
    if (dataSource.type == "repository") {
      auto mo = mDatabase->retrieveMO(dataSource.path, dataSource.name);
      TObject* obj = mo ? mo->getObject() : nullptr;
      if (obj) {
        mReductors[dataSource.name]->update(obj);
      }
    } else if (dataSource.type == "repository-quality") {
      auto qo = mDatabase->retrieveQO(dataSource.path + "/" + dataSource.name);
      if (qo) {
        mReductors[dataSource.name]->update(qo.get());
      }
    } else {
      ILOGE << "Unknown type of data source '" << dataSource.type << "'.";
    }
  }

  mTrend->Fill();
}

void TrendingTask::storePlots()
{
  ILOG(Info) << "Generating and storing " << mConfig.plots.size() << " plots." << ENDM;

  // why generate and store plots in the same function? because it is easier to handle the lifetime of pointers to the ROOT objects
  for (const auto& plot : mConfig.plots) {

    TCanvas* c = new TCanvas();

    mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), plot.option.c_str());

    c->SetName(plot.name.c_str());
    c->SetTitle(plot.title.c_str());

    // Postprocessing the plot - adding specified titles, configuring time-based plots, flushing buffers.
    // Notice that axes and title is drawn using a histogram, even in the case of graphs.
    if (auto histo = dynamic_cast<TH1*>(c->GetPrimitive("htemp"))) {
      // The title of histogram is printed, not the title of canvas => we set it as well.
      histo->SetTitle(plot.title.c_str());
      // We have to update the canvas to make the title appear.
      c->Update();

      // After the update, the title has a different size and it is not in the center anymore. We have to fix that.
      if (auto title = dynamic_cast<TPaveText*>(c->GetPrimitive("title"))) {
        title->SetBBoxCenterX(c->GetBBoxCenter().fX);
        // It will have an effect only after invoking Draw again.
        title->Draw();
      } else {
        ILOG(Info) << "Could not get the title TPaveText of the plot '" << plot.name << "'." << ENDM;
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
      }
      // QCG doesn't empty the buffers before visualizing the plot, nor does ROOT when saving the file,
      // so we have to do it here.
      histo->BufferEmpty();
    } else {
      ILOG(Info) << "Could not get the htemp histogram of the plot '" << plot.name << "'." << ENDM;
    }

    auto mo = std::make_shared<MonitorObject>(c, mConfig.taskName, mConfig.detectorName);
    mo->setIsOwner(false);
    mDatabase->storeMO(mo);

    // It should delete everything inside. Confirmed by trying to delete histo after and getting a segfault.
    delete c;
  }
}
