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
/// \file    TrendingRate.cxx
/// \author  Nicolò Jacazio nicolo.jacazio@cern.ch
/// \author  Francesco Noferini francesco.noferini@cern.ch
/// \author  Francesca Ercolessi francesca.ercolessi@cern.ch
/// \brief   Trending of the TOF interaction rate
/// \since   06/06/2022
///

#include "TOF/TrendingRate.h"
#include "TOFBase/Geo.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/RootClassFactory.h"
#include <boost/property_tree/ptree.hpp>
#include <TH1.h>
#include <TMath.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TDatime.h>
#include <TGraphErrors.h>
#include <TProfile.h>
#include <TPoint.h>

using namespace o2::tof;
using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::tof;

void TrendingRate::configure(std::string name, const boost::property_tree::ptree& config)
{
  mConfig = TrendingConfigTOF(name, config);
}

void TrendingRate::computeTOFRates(TH2F* h, TProfile* hp, std::vector<int>& bcInt, std::vector<float>& bcRate, std::vector<float>& bcPileup)
{
  if (!h) {
    ILOG(Warning, Support) << "Got no histogram, skipping" << ENDM;
    return;
  }
  if (!hp) {
    ILOG(Warning, Support) << "Got no profile, skipping" << ENDM;
    return;
  }
  TH1D* hback = nullptr;

  // Counting background
  int nb = 0;
  for (int i = 1; i <= hp->GetNbinsX(); i++) {
    if (hp->GetBinContent(i) < mThresholdBkg) {
      if (!hback) {
        hback = h->ProjectionY("back", i, i);
      } else {
        hback->Add(h->ProjectionY("tmp", i, i));
      }
      nb++;
    }
  }

  if (mActiveChannels) {
    mNoiseRatePerChannel = (hback->GetMean() - 0.5) / orbit_lenght * h->GetNbinsX() / mActiveChannels;
  }

  int ns = 0;
  std::vector<int> signals;

  float sumw = 0.f;
  float pilup = 0.f;
  float ratetot = 0.f;

  if (nb) {
    for (int i = 1; i <= h->GetNbinsX(); i++) {
      if (hp->GetBinContent(i) > mThresholdSgn) {
        signals.push_back(i);
        ns++;
      }
    }

    for (int i = 0; i < signals.size(); i++) {
      TH1D* hb = new TH1D(*hback);
      hb->SetName(Form("hback_%d", i));
      const int ibc = signals[i];
      const int bcmin = (ibc - 1) * 18;
      const int bcmax = ibc * 18;
      TH1D* hs = h->ProjectionY(Form("sign_%d_%d", bcmin, bcmax), ibc, ibc);
      hs->SetTitle(Form("%d < BC < %d", bcmin, bcmax));
      hb->Scale(hs->GetBinContent(1) / hb->GetBinContent(1));
      const float overall = hs->Integral();
      if (overall <= 0) {
        ILOG(Info, Support) << "no signal for BC index " << ibc << ENDM;
        continue;
      }
      const float background = hb->Integral();
      if (background <= 0) {
        ILOG(Info, Support) << "no background for BC index " << ibc << ENDM;
        continue;
      }
      const float prob = (overall - background) / overall;
      const float mu = TMath::Log(1.f / (1.f - prob));
      const float rate = mu / orbit_lenght;
      bcInt.push_back(ibc);
      bcRate.push_back(rate);
      bcPileup.push_back(mu / prob);
      sumw += rate;
      pilup += mu / prob * rate;
      ratetot += rate;
      if (prob > 0.f) {
        ILOG(Info, Support) << "interaction prob = " << mu << ", rate=" << rate << " Hz, mu=" << mu / prob << ENDM;
      }
      delete hb;
      delete hs;
    }

    if (sumw > 0) {
      mCollisionRate = ratetot;
      mPileup = pilup / sumw;
    }
    delete hback;
  }
}

void TrendingRate::initialize(Trigger, framework::ServiceRegistry&)
{
  // Setting parameters
  // This is not possible so far: TODO ask for parameters in trending!
  // if (auto param = mCustomParameters.find("ThresholdSgn"); param != mCustomParameters.end()) {
  //   mThresholdSgn = ::atof(param->second.c_str());
  // }
  // if (auto param = mCustomParameters.find("ThresholdBkg"); param != mCustomParameters.end()) {
  //   mThresholdBkg = ::atof(param->second.c_str());
  // }

  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("time", &mTime);
  mTrend->Branch("noiseRate", &mNoiseRatePerChannel);
  mTrend->Branch("collisionRate", &mCollisionRate);
  mTrend->Branch("activeChannels", &mActiveChannels);
  mTrend->Branch("pileup", &mPileup);

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<Reductor> reductor(root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }
  getObjectsManager()->startPublishing(mTrend.get());
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TrendingRate::update(Trigger t, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);
  generatePlots();
}

void TrendingRate::finalize(Trigger t, framework::ServiceRegistry&)
{
  generatePlots();
}

void TrendingRate::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  mTime = t.timestamp / 1000; // ROOT expects seconds since epoch
  // todo get run number when it is available. consider putting it inside monitor object's metadata (this might be not
  //  enough if we trend across runs).
  mMetaData.runNumber = -1;

  mActiveChannels = o2::tof::Geo::NCHANNELS;
  std::shared_ptr<o2::quality_control::core::MonitorObject> moHistogramMultVsBC = nullptr;
  std::shared_ptr<o2::quality_control::core::MonitorObject> moProfileMultVsBC = nullptr;

  std::vector<int> bcInt;
  std::vector<float> bcRate;
  std::vector<float> bcPileup;

  for (auto& dataSource : mConfig.dataSources) {
    auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
    if (!mo) {
      continue;
    }
    ILOG(Debug, Support) << "Got MO " << mo << ENDM;
    if (dataSource.name == "HitMap") {
      TH2F* hmap = (TH2F*)mo->getObject();
      hmap->Divide(hmap);
      mActiveChannels = hmap->Integral() * 24;
      ILOG(Info, Support) << "N channels = " << mActiveChannels << ENDM;
    } else if (dataSource.name == "Multiplicity/VsBC") {
      moHistogramMultVsBC = mo;
    } else if ("Multiplicity/VsBCpro") {
      moProfileMultVsBC = mo;
    }
  }

  if (mActiveChannels < 1) {
    ILOG(Info, Support) << "No active channels or empty objects";
    return;
  }

  if (!moHistogramMultVsBC) {
    ILOG(Info, Support) << "Got no histogramMultVsBC, can't compute rates";
    return;
  }
  if (!moProfileMultVsBC) {
    ILOG(Info, Support) << "Got no profileMultVsBC, can't compute rates";
    return;
  }

  computeTOFRates((TH2F*)moHistogramMultVsBC->getObject(), (TProfile*)moProfileMultVsBC->getObject(), bcInt, bcRate, bcPileup);

  ILOG(Info, Support) << "In " << mActiveChannels << " channels, noise rate per channel= " << mNoiseRatePerChannel << " Hz - collision rate = " << mCollisionRate << " Hz - mu-pilup = " << mPileup << ENDM;

  for (int i = 0; i < bcInt.size(); i++) {
    ILOG(Info, Support) << "bc = " << bcInt[i] * 18 - 9 << ") rate = " << bcRate[i] << ", pilup = " << bcPileup[i] << ENDM;
  }

  mTrend->Fill();
}

void TrendingRate::generatePlots()
{
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

    TCanvas* c = new TCanvas();

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
      ILOG(Error, Devel) << "Could not get the htemp histogram of the plot '" << plot.name << "'." << ENDM;
    }

    mPlots[plot.name] = c;
    getObjectsManager()->startPublishing(c);
  }
}
