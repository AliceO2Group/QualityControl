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
/// \file   RatesPlotter.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_RATESPLOTTER_H
#define QC_MODULE_RATESPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCH/GlobalHistogram.h"
#include "MCH/TH2ElecMapReductor.h"
#include "MCHRawElecMap/Mapper.h"
#include <TCanvas.h>

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class RatesPlotter : public HistPlotter
{
 public:
  RatesPlotter(std::string path, TH2F* hRef, float rateMin, float rateMax, bool fullPlots = false);

  void update(TH2F* hRates);

 private:
  void addHisto(TH1* h, bool statBox, const char* drawOptions, const char* displayHints)
  {
    h->SetOption(drawOptions);
    if (!statBox) {
      h->SetStats(0);
    }
    histograms().emplace_back(HistInfo{ h, drawOptions, displayHints });
  }

  void addCanvas(TCanvas* c, TH1* h, bool statBox, const char* drawOptions, const char* displayHints)
  {
    h->SetOption(drawOptions);
    if (!statBox) {
      h->SetStats(0);
    }
    histograms().emplace_back(HistInfo{ c, "", displayHints });
  }

  void fillAverageHistos(TH2F* h);
  void fillGlobalHistos(TH2F* h);

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;

  std::unique_ptr<TH2ElecMapReductor> mElecMapReductor;

  std::unique_ptr<TH1F> mHistogramMeanRatePerDE;
  std::unique_ptr<TH1F> mHistogramMeanRatePerDERef;
  std::unique_ptr<TH1F> mHistogramMeanRateRefRatio;
  std::unique_ptr<TCanvas> mCanvasMeanRatePerDE;

  std::unique_ptr<TH1F> mHistogramGoodChannelsFractionPerDE;
  std::unique_ptr<TH1F> mHistogramGoodChannelsFractionPerDERef;
  std::unique_ptr<TH1F> mHistogramGoodChannelsFractionRefRatio;
  std::unique_ptr<TCanvas> mCanvasGoodChannelsFractionPerDE;

  std::map<int, std::shared_ptr<DetectorHistogram>> mHistogramRateDE[2]; // 2D hit rate map for each DE
  std::shared_ptr<GlobalHistogram> mHistogramRateGlobal[2];              // Rate histogram (global XY view)
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_RATESPLOTTER_H
