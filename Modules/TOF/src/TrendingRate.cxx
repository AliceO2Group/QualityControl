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
#include "QualityControl/ActivityHelpers.h"
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

void TrendingRate::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingConfigTOF(getID(), config);
  mThresholdSgn = mConfig.mConfigTrendingRate.thresholdSignal;
  mThresholdBkg = mConfig.mConfigTrendingRate.thresholdBackground;
}

void TrendingRate::computeTOFRates(TH2F* h, std::vector<int>& bcInt, std::vector<float>& bcRate, std::vector<float>& bcPileup)
{

  mCollisionRate = 0.;
  mPileupRate = 0.;
  mNoiseRatePerChannel = 0.;
  mNIBC = 0;

  if (!h) {
    ILOG(Warning, Support) << "Got no histogram, skipping" << ENDM;
    return;
  }
  TH1D* hback = nullptr;

  if (!mPreviousPlot) {
    // create object for comparing with the past since
    mPreviousPlot = new TH2F(*h);
    mPreviousPlot->SetName("hTOFprevIRmap");
    mPreviousPlot->Reset();
  }

  TH2F hDiffGlobal(*h);
  hDiffGlobal.SetName("hTOFhitDiff");
  hDiffGlobal.Add(mPreviousPlot, -1);

  TProfile* hpdiff = hDiffGlobal.ProfileX("hProTOFhitDiff");

  // Counting background
  int nb = 0;
  for (int i = 1; i <= hpdiff->GetNbinsX(); i++) {
    if (hpdiff->GetBinContent(i) - 0.5 < mThresholdBkg) {
      if (!hback) {
        hback = hDiffGlobal.ProjectionY("back", i, i);
      } else {
        hback->Add(hDiffGlobal.ProjectionY("tmp", i, i));
      }
      nb++;
    }
  }

  // update previous object status
  mPreviousPlot->Reset();
  mPreviousPlot->Add(h);

  if (nb == 0) { // threshold too low? return since hback was not created
    ILOG(Warning, Support) << "Counted 0 background events, BKG threshold might be too low!" << ENDM;
    delete hpdiff;
    return;
  }
  if (hback->Integral() < 0 || hback->GetMean() < 0.5) {
    return;
  }

  if (mActiveChannels > 0) {
    mNoiseRatePerChannel = (hback->GetMean() - 0.5) / orbit_lenght * h->GetNbinsX() / mActiveChannels;
  }

  std::vector<int> signals;

  float sumw = 0.f;
  float pilup = 0.f;
  float ratetot = 0.f;

  if (nb) {
    for (int i = 1; i <= hDiffGlobal.GetNbinsX(); i++) {
      if (hpdiff->GetBinContent(i) - 0.5 > mThresholdSgn) {
        signals.push_back(i);
        mNIBC++;
      }
    }

    for (int i = 0; i < signals.size(); i++) {
      TH1D* hb = new TH1D(*hback);
      hb->SetName(Form("hback_%d", i));
      const int ibc = signals[i];
      const int bcmin = (ibc - 1) * 18;
      const int bcmax = ibc * 18;
      TH1D* hs = hDiffGlobal.ProjectionY(Form("sign_%d_%d", bcmin, bcmax), ibc, ibc);
      hs->SetTitle(Form("%d < BC < %d", bcmin, bcmax));
      if (hb->GetBinContent(1)) {
        hb->Scale(hs->GetBinContent(1) / hb->GetBinContent(1));
      } else {
        continue;
      }
      const float overall = hs->Integral();
      if (overall <= 0.f) {
        ILOG(Info, Support) << "no signal for BC index " << ibc << ENDM;
        continue;
      }
      const float background = hb->Integral();
      if (background <= 0.f) {
        ILOG(Info, Support) << "no background for BC index " << ibc << ENDM;
        continue;
      }
      const float prob = (overall - background) / overall;
      if ((1.f - prob) < 0.f) {
        ILOG(Info, Support) << "Probability is 1, can't comute mu" << ENDM;
        continue;
      }
      const float mu = TMath::Log(1.f / (1.f - prob));
      const float rate = mu / orbit_lenght;
      bcInt.push_back(ibc);
      bcRate.push_back(rate);
      if (prob <= 0.f) {
        ILOG(Warning, Support) << "Probability is 0, can't compute pileup" << ENDM;
        continue;
      }
      bcPileup.push_back(mu / prob);
      sumw += rate;
      pilup += mu / prob * rate;
      ratetot += rate;
      ILOG(Info, Support) << "interaction prob = " << mu << ", rate=" << rate << " Hz, mu=" << mu / prob << ENDM;
      delete hb;
      delete hs;
    }

    if (sumw > 0) {
      mCollisionRate = ratetot;
      mPileupRate = pilup / sumw;
    }
    delete hback;
  }

  delete hpdiff;
}

void TrendingRate::initialize(Trigger, framework::ServiceRegistryRef)
{
  // Setting parameters

  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("time", &mTime);
  mTrend->Branch("noiseRate", &mNoiseRatePerChannel);
  mTrend->Branch("collisionRate", &mCollisionRate);
  mTrend->Branch("activeChannels", &mActiveChannels);
  mTrend->Branch("pileup", &mPileupRate);
  mTrend->Branch("nIBC", &mNIBC);

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<Reductor> reductor(root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }
  getObjectsManager()->startPublishing(mTrend.get());
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TrendingRate::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);
  generatePlots();
}

void TrendingRate::finalize(Trigger t, framework::ServiceRegistryRef)
{
  generatePlots();
}

void TrendingRate::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  mTime = activity_helpers::isLegacyValidity(t.activity.mValidity)
            ? t.timestamp / 1000
            : t.activity.mValidity.getMax() / 1000; // ROOT expects seconds since epoch.
  mMetaData.runNumber = t.activity.mId;

  mActiveChannels = o2::tof::Geo::NCHANNELS;
  std::shared_ptr<o2::quality_control::core::MonitorObject> moHistogramMultVsBC = nullptr;

  std::vector<int> bcInt;
  std::vector<float> bcRate;
  std::vector<float> bcPileup;

  bool foundHitMap = false;
  bool foundVsBC = false;

  for (auto& dataSource : mConfig.dataSources) {
    auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
    if (!mo) {
      continue;
    }
    ILOG(Debug, Support) << "Got MO " << mo << ENDM;
    if (dataSource.name == "HitMap") {
      foundHitMap = true;
      TH2F* hmap = dynamic_cast<TH2F*>(mo->getObject());
      hmap->Divide(hmap);
      mActiveChannels = hmap->Integral() * 24;
      ILOG(Info, Support) << "N channels = " << mActiveChannels << ENDM;
    } else if (dataSource.name == "Multiplicity/VsBC") {
      moHistogramMultVsBC = mo;
      foundVsBC = true;
    }
  }

  if (!foundHitMap) {
    ILOG(Info, Support) << "HitMap not found";
    return;
  }

  if (!foundVsBC) {
    ILOG(Info, Support) << "Multiplicity/VsBC not found";
    return;
  }

  if (mActiveChannels < 1) {
    ILOG(Info, Support) << "No active channels or empty objects";
    return;
  }

  computeTOFRates(dynamic_cast<TH2F*>(moHistogramMultVsBC->getObject()), bcInt, bcRate, bcPileup);

  ILOG(Info, Support) << "In " << mActiveChannels << " channels, noise rate per channel= " << mNoiseRatePerChannel << " Hz - collision rate = " << mCollisionRate << " Hz - mu-pilup = " << mPileupRate << ENDM;

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
    TCanvas* c;
    if (!mPlots.count(plot.name)) {
      c = new TCanvas(plot.name.c_str(), plot.title.c_str());
      mPlots[plot.name] = c;
      getObjectsManager()->startPublishing(c);
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
  }
}
