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
/// \file   ClusterSizePlotter.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_CLUSTERSIZEPLOTTER_H
#define QC_MODULE_CLUSTERSIZEPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCH/ClusterSizeReductor.h"
#include "MCH/Helpers.h"
#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TLegend.h>

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class ClusterSizePlotter : public HistPlotter
{
 public:
  ClusterSizePlotter(std::string path, TH2F* hRef, bool fullPlots = false);

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

  void fillHistograms(TH2F* h);

  std::string mPath;

  std::unique_ptr<ClusterSizeReductor> mClusterSizeReductor;

  std::array<std::unique_ptr<TH1F>, 3> mHistogramClusterSizePerDE;
  std::array<std::unique_ptr<TH1F>, 3> mHistogramClusterSizePerDERef;
  std::array<std::unique_ptr<TH1F>, 3> mHistogramClusterSizePerDERefRatio;
  std::array<std::unique_ptr<TCanvas>, 3> mCanvasClusterSizePerDE;
  std::array<std::unique_ptr<TH1F>, getNumDE() * 3> mHistogramClusterSize;
  std::array<std::unique_ptr<TLegend>, getNumDE()> mLegendClusterSize;
  std::array<std::unique_ptr<TCanvas>, getNumDE()> mCanvasClusterSize;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_CLUSTERSIZEPLOTTER_H
