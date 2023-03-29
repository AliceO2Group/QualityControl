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
/// \file   ClusterChargePlotter.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_CHARGEPLOTTER_H
#define QC_MODULE_CHARGEPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCH/ClusterChargeReductor.h"
#include "MCH/Helpers.h"
#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class ClusterChargePlotter : public HistPlotter
{
 public:
  ClusterChargePlotter(std::string path, TH2F* hRef, bool fullPlots = false);

  void update(TH2F* hCharge);

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

  std::unique_ptr<ClusterChargeReductor> mChargeReductor;

  std::unique_ptr<TH1F> mHistogramChargePerDE;
  std::unique_ptr<TH1F> mHistogramChargePerDERef;
  std::unique_ptr<TH1F> mHistogramChargeRefRatio;
  std::unique_ptr<TCanvas> mCanvasChargePerDE;
  std::array<std::unique_ptr<TH1F>, getNumDE()> mHistogramCharge;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_CHARGEPLOTTER_H
