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
/// \file    TrendingFECHistRatio.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Trending of the MCH FEC histogram ratios (rates, efficiencies, etc...)
/// \since   06/06/2022
///

#include "MCH/TrendingFECHistRatio.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MCH/GlobalHistogram.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"
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
#include "MCHConstants/DetectionElements.h"

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::muon;
using namespace o2::quality_control_modules::muonchambers;

void TrendingFECHistRatio::configure(const boost::property_tree::ptree& config)
{
  mConfig = TrendingConfigMCH(getID(), config);
  for (int chamber = 0; chamber < 10; chamber++) {
    mConfig.plots.push_back({ fmt::format("{}_CH{}", mConfig.mConfigTrendingFECHistRatio.namePrefix, chamber + 1),
                              fmt::format("{} CH{}", mConfig.mConfigTrendingFECHistRatio.namePrefix, chamber + 1),
                              fmt::format("CH{}:time", chamber + 1),
                              "", "*L", "" });
  }
  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    mConfig.plots.push_back({ fmt::format("{}{}_{}", getHistoPath(de), mConfig.mConfigTrendingFECHistRatio.namePrefix, de),
                              fmt::format("{} DE{}", mConfig.mConfigTrendingFECHistRatio.namePrefix, de),
                              fmt::format("DE{}:time", de),
                              "", "*L", "" });
  }
}

int TrendingFECHistRatio::checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel)
{
  uint16_t solarId = -1;
  int deId = -1;
  int dsIddet = -1;
  int padId = -1;

  o2::mch::raw::FeeLinkId feeLinkId{ feeId, linkId };

  if (auto opt = mFeeLink2SolarMapper(feeLinkId); opt.has_value()) {
    solarId = opt.value();
  }
  if (solarId < 0 || solarId > 1023) {
    return -1;
  }

  o2::mch::raw::DsElecId dsElecId{ solarId, static_cast<uint8_t>(eLinkId / 5), static_cast<uint8_t>(eLinkId % 5) };

  if (auto opt = mElec2DetMapper(dsElecId); opt.has_value()) {
    o2::mch::raw::DsDetId dsDetId = opt.value();
    dsIddet = dsDetId.dsId();
    deId = dsDetId.deId();
  }

  if (deId < 0 || dsIddet < 0) {
    return -1;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  padId = segment.findPadByFEE(dsIddet, int(channel));

  if (padId < 0) {
    return -1;
  }
  return deId;
}

void TrendingFECHistRatio::computeMCHFECHistRatios(TH2F* hNum, TH2F* hDen)
{

  for (int chamber = 0; chamber < 10; chamber++) {
    mTrendCH[chamber] = 0;
  }
  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    mTrendDE[de] = 0;
  }

  if (!hNum || !hDen) {
    ILOG(Warning, Support) << "Got no histogram, skipping" << ENDM;
    return;
  }

  if (!mPreviousNum) {
    // create object for comparing with the past since
    mPreviousNum = new TH2F(*hNum);
    mPreviousNum->SetName("hMCHprevNum");
    mPreviousNum->Reset();
  }

  if (!mPreviousDen) {
    // create object for comparing with the past since
    mPreviousDen = new TH2F(*hDen);
    mPreviousDen->SetName("hMCHprevDen");
    mPreviousDen->Reset();
  }

  TH2F hDiffNum(*hNum);
  hDiffNum.SetName("hMCHdiffNum");
  hDiffNum.Add(mPreviousNum, -1);

  TH2F hDiffDen(*hDen);
  hDiffDen.SetName("hMCHdiffDen");
  hDiffDen.Add(mPreviousDen, -1);

  hDiffNum.Divide(&hDiffDen);

  std::array<double, 10> chamberNum;
  std::fill(chamberNum.begin(), chamberNum.end(), 0);
  std::array<double, 10> chamberDen;
  std::fill(chamberDen.begin(), chamberDen.end(), 0);

  std::array<double, 1100> deNum;
  std::fill(deNum.begin(), deNum.end(), 0);
  std::array<double, 1100> deDen;
  std::fill(deDen.begin(), deDen.end(), 0);

  int nbinsx = hDiffNum.GetXaxis()->GetNbins();
  int nbinsy = hDiffNum.GetYaxis()->GetNbins();
  int ngood = 0;
  int npads = 0;
  for (int i = 1; i <= nbinsx; i++) {
    int index = i - 1;
    int ds_addr = (index % 40);
    int link_id = (index / 40) % 12;
    int fee_id = index / (12 * 40);

    for (int j = 1; j <= nbinsy; j++) {
      int chan_addr = j - 1;

      // skip bins with no statistics
      if (hDiffDen.GetBinContent(i, j) == 0) {
        continue;
      }

      int de = checkPadMapping(fee_id, link_id, ds_addr, chan_addr);
      if (de < 0) {
        continue;
      }
      deNum[de] += hDiffNum.GetBinContent(i, j);
      deDen[de] += 1;

      int chamber = de / 100 - 1;
      chamberNum[chamber] += hDiffNum.GetBinContent(i, j);
      chamberDen[chamber] += 1;
    }
  }

  // compute and store average rate for each detection element
  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    double ratio = (deDen[de] > 0) ? (deNum[de] / deDen[de]) : 0;
    mTrendDE[de] = ratio;
  }

  // compute and store average rate for each chamber
  for (int ch = 0; ch < 10; ch++) {
    double ratio = (chamberDen[ch] > 0) ? (chamberNum[ch] / chamberDen[ch]) : 0;
    mTrendCH[ch] = ratio;
  }

  // update previous object status
  mPreviousNum->Reset();
  mPreviousNum->Add(hNum);
  mPreviousDen->Reset();
  mPreviousDen->Add(hDen);
}

void TrendingFECHistRatio::initialize(Trigger, framework::ServiceRegistryRef)
{
  mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();

  // Setting parameters

  // Preparing data structure of TTree
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("runNumber", &mMetaData.runNumber);
  mTrend->Branch("time", &mTime);

  for (int chamber = 0; chamber < 10; chamber++) {
    mTrend->Branch(fmt::format("CH{}", chamber + 1).c_str(), &mTrendCH[chamber]);
  }
  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    mTrend->Branch(fmt::format("DE{}", de).c_str(), &mTrendDE[de]);
  }

  for (const auto& source : mConfig.dataSources) {
    std::unique_ptr<Reductor> reductor(root_class_factory::create<Reductor>(source.moduleName, source.reductorName));
    mTrend->Branch(source.name.c_str(), reductor->getBranchAddress(), reductor->getBranchLeafList());
    mReductors[source.name] = std::move(reductor);
  }
  getObjectsManager()->startPublishing(mTrend.get());
}

// todo: see if OptimizeBaskets() indeed helps after some time
void TrendingFECHistRatio::update(Trigger t, framework::ServiceRegistryRef services)
{
  std::cout << "TrendingFECHistRatio::update()" << std::endl;
  auto& qcdb = services.get<repository::DatabaseInterface>();

  trendValues(t, qcdb);
  generatePlots();
}

void TrendingFECHistRatio::finalize(Trigger t, framework::ServiceRegistryRef)
{
  generatePlots();
}

void TrendingFECHistRatio::trendValues(const Trigger& t, repository::DatabaseInterface& qcdb)
{
  mTime = t.timestamp / 1000; // ROOT expects seconds since epoch
  mMetaData.runNumber = t.activity.mId;

  std::shared_ptr<o2::quality_control::core::MonitorObject> moHistogramRates = nullptr;

  std::vector<int> bcInt;
  std::vector<float> bcRate;
  std::vector<float> bcPileup;

  for (auto& dataSource : mConfig.dataSources) {
    auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
    if (!mo) {
      continue;
    }
    ILOG(Debug, Support) << "Got MO " << mo << ENDM;
    moHistogramRates = mo;
  }

  if (!moHistogramRates) {
    ILOG(Info, Support) << "Got no histogramRates, can't compute rates";
    return;
  }

  MergeableTH2Ratio* hr = dynamic_cast<MergeableTH2Ratio*>(moHistogramRates->getObject());
  if (!hr) {
    ILOG(Info, Support) << "Cannot cast histogramRates, can't compute rates";
    return;
  }
  computeMCHFECHistRatios(hr->getNum(), hr->getDen());

  mTrend->Fill();
}

void TrendingFECHistRatio::generatePlots()
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
