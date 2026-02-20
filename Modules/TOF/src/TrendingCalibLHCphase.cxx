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
/// \file   TrendingCalibLHCphase.cxx
/// \author Francesca Ercolessi

#include "TOF/TrendingCalibLHCphase.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/ActivityHelpers.h"
#include "CCDB/BasicCCDBManager.h"
#include "QualityControl/UserCodeInterface.h"
#include "DetectorsCalibration/Utils.h"
#include "TOFBase/Utils.h"
#include "TOFCalibration/LHCClockCalibrator.h"
#include "TOFBase/Geo.h"
#include "CCDB/CcdbApi.h"

#include <TH1.h>
#include <TMath.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TDatime.h>
#include <TGraphErrors.h>
#include <TProfile.h>
#include <TPoint.h>

#include <boost/property_tree/ptree.hpp>

using namespace std;
using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::tof;

using LHCphase = o2::dataformats::CalibLHCphaseTOF;

void TrendingCalibLHCphase::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingConfigTOF(getID(), config);
  mHost = config.get<std::string>("qc.postprocessing." + getID() + ".dataSourceURL");
}

void TrendingCalibLHCphase::initialize(Trigger, framework::ServiceRegistryRef)
{
  mCdbApi.init(mHost);

  // Preparing data structure of TTree
  mTrend.reset();
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("time", &mTime);
  mTrend->Branch("phase", &mPhase);
  mTrend->Branch("startValidity", &mStartValidity);
  mTrend->Branch("endValidity", &mEndValidity);

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<Reductor> reductor(root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }
  getObjectsManager()->startPublishing(mTrend.get(), PublicationPolicy::ThroughStop);
}

void TrendingCalibLHCphase::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& ccdb = services.get<repository::DatabaseInterface>();

  trendValues(t, ccdb);
  generatePlots();
}

void TrendingCalibLHCphase::finalize(Trigger t, framework::ServiceRegistryRef)
{
  generatePlots();
}

void TrendingCalibLHCphase::trendValues(const Trigger& t, repository::DatabaseInterface& ccdb)
{
  mTime = t.activity.mValidity.getMax() / 1000;
  mMetaData.runNumber = t.activity.mId;

  mPhase = 0.;
  mStartValidity = 0;
  mEndValidity = 0;

  map<string, string> metadata; // can be empty

  for (auto& dataSource : mConfig.dataSources) {

    if (dataSource.type == "ccdb") {

      auto calib_object = UserCodeInterface::retrieveConditionAny<LHCphase>(dataSource.path, metadata, t.timestamp);

      if (!calib_object) {
        ILOG(Error, Support) << "Could not retrieve calibration file '" << dataSource.path << "'." << ENDM;
      } else {
        ILOG(Info, Support) << "Retrieved calibration file '" << dataSource.path << "'." << ENDM;
        mPhase = calib_object->getLHCphase(0);
        mStartValidity = (double)calib_object->getStartValidity() * 0.001;
        mEndValidity = (double)calib_object->getEndValidity() * 0.001;
      }
    } else {
      ILOG(Error, Support) << "Unknown type of data source '" << dataSource.type << "'. Expected: ccdb" << ENDM;
    }
  }

  mTrend->Fill();
}

void TrendingCalibLHCphase::generatePlots()
{
  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, won't generate any plots." << ENDM;
    return;
  }

  ILOG(Info, Support) << "Generating " << mConfig.plots.size() << " plots." << ENDM;

  for (const auto& plot : mConfig.plots) {

    // Before we generate any new plots, we have to delete existing under the same names.
    // It seems that ROOT cannot handle an existence of two canvases with a common name in the same process.
    TCanvas* c;
    if (!mPlots.count(plot.name)) {
      c = new TCanvas(plot.name.c_str(), plot.title.c_str());
      mPlots[plot.name] = c;
      getObjectsManager()->startPublishing(c, PublicationPolicy::Forever);
    } else {
      c = (TCanvas*)mPlots[plot.name];
      c->cd();
    }

    // we determine the order of the plot, i.e. if it is a histogram (1), graph (2), or any higher dimension.
    const size_t plotOrder = std::count(plot.varexp.begin(), plot.varexp.end(), ':') + 1;
    // we have to delete the graph errors after the plot is saved, unfortunately the canvas does not take its ownership
    TGraphErrors* graphErrors = nullptr;

    mTrend->Draw(plot.varexp.c_str(), plot.selection.c_str(), plot.option.c_str());

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
        // We try to convince ROOT to delete graphErrors together with the rest of the canvas.
        if (auto* pad = c->GetPad(0)) {
          if (auto* primitives = pad->GetListOfPrimitives()) {
            primitives->Add(graphErrors);
          }
        }
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
        // It will have an effect only after invoking Draw again.
        title->Draw();
      } else {
        ILOG(Error, Devel) << "Could not get the title TPaveText of the plot '" << plot.name << "'." << ENDM;
      }

      // We have to explicitly configure showing time on x axis.
      // I hope that looking for ":time" is enough here and someone doesn't come with an exotic use-case.
      if (plot.varexp.find(":time") != std::string::npos || plot.varexp.find(":startValidity") != std::string::npos || plot.varexp.find(":endValidity") != std::string::npos) {
        histo->GetXaxis()->SetTimeDisplay(1);
        // It deals with highly congested dates labels
        histo->GetXaxis()->SetNdivisions(505);
        // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
        histo->GetXaxis()->SetTimeOffset(0.0);
        histo->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
      }
      if (plot.varexp.find("startValidity:") != std::string::npos || plot.varexp.find("endValidity:") != std::string::npos) {
        histo->GetYaxis()->SetTimeDisplay(1);
        // It deals with highly congested dates labels
        histo->GetYaxis()->SetNdivisions(505);
        // Without this it would show dates in order of 2044-12-18 on the day of 2019-12-19.
        histo->GetYaxis()->SetTimeOffset(0.0);
        histo->GetYaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
      }
      // QCG doesn't empty the buffers before visualizing the plot, nor does ROOT when saving the file,
      // so we have to do it here.
      histo->BufferEmpty();
    } else {
      ILOG(Error, Devel) << "Could not get the htemp histogram of the plot '" << plot.name << "'." << ENDM;
    }
  }
}
