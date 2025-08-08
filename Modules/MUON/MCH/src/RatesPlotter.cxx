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
/// \file   RatesPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/RatesPlotter.h"
#include "MUONCommon/Helpers.h"
#include "MCH/Helpers.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHGlobalMapping/DsIndex.h"
#include <fmt/format.h>

using namespace o2::mch;
using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

RatesPlotter::RatesPlotter(std::string path, float rateMin, float rateMax, bool perStationPlots, bool fullPlots)
{
  // mappers used for filling the histograms in detector coordinates
  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();

  // reductor for the rates plot in electronics coordinates
  mElecMapReductor = std::make_unique<TH2ElecMapReductor>(rateMin, rateMax);

  //----------------------------------
  // Rate distribution histograms
  //----------------------------------

  int nbins = 100;
  double xMax = rateMax * 10;
  double xMin = (rateMin > 0) ? rateMin / 10 : xMax / 1000000;
  auto xbins = makeLogBinning(xMin, xMax, nbins);
  std::vector<double> ybins{ 1, 2, 3, 4, 5, 6 };
  mHistogramRatePerStation = std::make_unique<TH2F>(TString::Format("%sRatesDistribution", path.c_str()),
                                                    "Rates distribution",
                                                    nbins, xbins.data(),
                                                    5, ybins.data());
  for (int i = 1; i <= mHistogramRatePerStation->GetYaxis()->GetNbins(); i++) {
    mHistogramRatePerStation->GetYaxis()->SetBinLabel(i, TString::Format("ST%d", i));
  }

  if (perStationPlots || fullPlots) {
    addHisto(mHistogramRatePerStation.get(), false, "colz", "logx");
  }

  //----------------------------------
  // Mean rates histograms
  //----------------------------------

  mHistogramMeanRatePerDE = std::make_unique<TH1F>(TString::Format("%sMeanRate", path.c_str()), "Mean Rate",
                                                   getNumDE(), 0, getNumDE());
  addDEBinLabels(mHistogramMeanRatePerDE.get());
  addHisto(mHistogramMeanRatePerDE.get(), false, "histo", "logy");

  mHistogramMeanRatePerSolar = std::make_unique<TH1F>(TString::Format("%sMeanRatePerSolar", path.c_str()), "Mean Rate per SOLAR board",
                                                      getNumSolar(), 0, getNumSolar());
  addSolarBinLabels(mHistogramMeanRatePerSolar.get());
  addHisto(mHistogramMeanRatePerSolar.get(), false, "histo", "logy");

  //----------------------------------
  // "Good" channels fraction histograms
  //----------------------------------

  mHistogramGoodChannelsFractionPerDE = std::make_unique<TH1F>(TString::Format("%sGoodChannelsFraction", path.c_str()),
                                                               "Good channels fraction", getNumDE(), 0, getNumDE());
  addDEBinLabels(mHistogramGoodChannelsFractionPerDE.get());
  addHisto(mHistogramGoodChannelsFractionPerDE.get(), false, "histo", "");

  mHistogramGoodChannelsFractionPerSolar = std::make_unique<TH1F>(TString::Format("%sGoodChannelsFractionPerSolar", path.c_str()),
                                                                  "Good channels fraction per SOLAR board", getNumSolar(), 0, getNumSolar());
  addSolarBinLabels(mHistogramGoodChannelsFractionPerSolar.get());
  addHisto(mHistogramGoodChannelsFractionPerSolar.get(), false, "histo", "");

  //--------------------------------------------------
  // Rates histograms in global detector coordinates
  //--------------------------------------------------

  mHistogramRateGlobal[0] = std::make_shared<GlobalHistogram>(fmt::format("{}Rate_ST12", path), "ST12 Rate", 0, 5);
  mHistogramRateGlobal[0]->init();
  addHisto(mHistogramRateGlobal[0]->getHist(), false, "colz", "colz");

  mHistogramRateGlobal[1] = std::make_shared<GlobalHistogram>(fmt::format("{}Rate_ST345", path), "ST345 Rate", 1, 10);
  mHistogramRateGlobal[1]->init();
  addHisto(mHistogramRateGlobal[1]->getHist(), false, "colz", "colz");

  //--------------------------------------------------
  // Rates histograms in detector coordinates
  //--------------------------------------------------

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    auto h = std::make_shared<DetectorHistogram>(TString::Format("%s%sRate_XY_B_%03d", path.c_str(), getHistoPath(de).c_str(), de),
                                                 TString::Format("Hit Rate (DE%03d B)", de), de, int(0));
    mHistogramRateDE[0].insert(make_pair(de, h));
    if (fullPlots) {
      addHisto(h->getHist(), false, "colz", "colz");
    }

    h = std::make_shared<DetectorHistogram>(TString::Format("%s%sRate_XY_NB_%03d", path.c_str(), getHistoPath(de).c_str(), de),
                                            TString::Format("Hit Rate (DE%03d NB)", de), de, int(1));
    mHistogramRateDE[1].insert(make_pair(de, h));
    if (fullPlots) {
      addHisto(h->getHist(), false, "colz", "colz");
    }
  }
}

//_________________________________________________________________________________________

void RatesPlotter::fillAverageHistos(TH2F* hRates)
{
  // extract the integrated average occupancies
  mElecMapReductor->update(hRates);
  for (size_t de = 0; de < mHistogramMeanRatePerDE->GetXaxis()->GetNbins(); de++) {
    mHistogramMeanRatePerDE->SetBinContent(de + 1, mElecMapReductor->getDeValue(de));
    mHistogramMeanRatePerDE->SetBinError(de + 1, 0.1);
  }

  for (size_t de = 0; de < mHistogramGoodChannelsFractionPerDE->GetXaxis()->GetNbins(); de++) {
    float nPads = mElecMapReductor->getNumPads(de);
    float nPadsBad = mElecMapReductor->getNumPadsBad(de) + mElecMapReductor->getNumPadsNoStat(de);
    float nPadsGood = nPads - nPadsBad;
    if (nPads > 0) {
      mHistogramGoodChannelsFractionPerDE->SetBinContent(de + 1, nPadsGood / nPads);
      mHistogramGoodChannelsFractionPerDE->SetBinError(de + 1, 0.1);
    } else {
      mHistogramGoodChannelsFractionPerDE->SetBinContent(de + 1, 0);
      mHistogramGoodChannelsFractionPerDE->SetBinError(de + 1, 1);
    }
  }

  for (size_t solar = 0; solar < mHistogramMeanRatePerSolar->GetXaxis()->GetNbins(); solar++) {
    mHistogramMeanRatePerSolar->SetBinContent(solar + 1, mElecMapReductor->getSolarValue(solar));
    mHistogramMeanRatePerSolar->SetBinError(solar + 1, 0.1);
  }

  for (size_t solar = 0; solar < mHistogramGoodChannelsFractionPerSolar->GetXaxis()->GetNbins(); solar++) {
    float nPads = mElecMapReductor->getSolarNumPads(solar);
    float nPadsBad = mElecMapReductor->getSolarNumPadsBad(solar) + mElecMapReductor->getSolarNumPadsNoStat(solar);
    float nPadsGood = nPads - nPadsBad;
    if (nPads > 0) {
      mHistogramGoodChannelsFractionPerSolar->SetBinContent(solar + 1, nPadsGood / nPads);
      mHistogramGoodChannelsFractionPerSolar->SetBinError(solar + 1, 0.1);
    } else {
      mHistogramGoodChannelsFractionPerSolar->SetBinContent(solar + 1, 0);
      mHistogramGoodChannelsFractionPerSolar->SetBinError(solar + 1, 1);
    }
  }
}

//_________________________________________________________________________________________

void RatesPlotter::fillGlobalHistos(TH2F* h)
{
  if (!h) {
    return;
  }

  // the rates distribution is not a cumulative plot,
  // clear it before updating it with the new values
  mHistogramRatePerStation->Reset();

  // loop over bins in electronics coordinates, and map the channels to the corresponding cathode pads
  int nbinsx = h->GetXaxis()->GetNbins();
  int nbinsy = h->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    // address of the DS board in detector representation
    auto dsDetId = getDsDetId(i - 1);
    auto deId = dsDetId.deId();
    auto dsId = dsDetId.dsId();
    auto chamberId = deId / 100;
    auto stationId = ((chamberId - 1) / 2) + 1;

    if (stationId < 1 || stationId > 5) {
      continue;
    }

    for (int j = 1; j <= nbinsy; j++) {
      int channel = j - 1;
      int padId = -1;

      const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
      padId = segment.findPadByFEE(dsId, int(channel));

      if (padId < 0) {
        continue;
      }

      double rate = h->GetBinContent(i, j);

      if (rate <= mHistogramRatePerStation->GetXaxis()->GetXmin()) {
        rate = mHistogramRatePerStation->GetXaxis()->GetBinCenter(1);
      }
      if (rate >= mHistogramRatePerStation->GetXaxis()->GetXmax()) {
        auto nbins = mHistogramRatePerStation->GetXaxis()->GetNbins();
        rate = mHistogramRatePerStation->GetXaxis()->GetBinCenter(nbins);
      }
      mHistogramRatePerStation->Fill(rate, stationId);

      double padX = segment.padPositionX(padId);
      double padY = segment.padPositionY(padId);
      float padSizeX = segment.padSizeX(padId);
      float padSizeY = segment.padSizeY(padId);
      int cathode = segment.isBendingPad(padId) ? 0 : 1;

      // Fill 2D rate histograms
      auto hRate = mHistogramRateDE[cathode].find(deId);
      if ((hRate != mHistogramRateDE[cathode].end()) && (hRate->second != NULL)) {
        hRate->second->Set(padX, padY, padSizeX, padSizeY, rate);
      }
    }
  }

  mHistogramRateGlobal[0]->set(mHistogramRateDE[0], mHistogramRateDE[1]);
  mHistogramRateGlobal[1]->set(mHistogramRateDE[0], mHistogramRateDE[1]);
}

//_________________________________________________________________________________________

void RatesPlotter::update(TH2F* hRates)
{
  fillAverageHistos(hRates);
  fillGlobalHistos(hRates);
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
