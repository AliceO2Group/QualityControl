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
/// \file   RatesTrendsPlotter.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_RATESTRENDSPLOTTER_H
#define QC_MODULE_RATESTRENDSPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCH/Helpers.h"
#include "MCH/TH2ElecMapReductor.h"
#include <TCanvas.h>

class TH2F;

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class RatesTrendsPlotter : public HistPlotter
{
 public:
  RatesTrendsPlotter(std::string path, TH2F* hRef, bool fullPlots = false);

  void update(long time, TH2F* hEfficiency);

 private:
  void addCanvas(TCanvas* c, const char* displayHints)
  {
    histograms().emplace_back(HistInfo{ c, "", displayHints });
  }

  long mTime{ 0 };
  std::string mPath;

  // Data reductor
  std::unique_ptr<TH2ElecMapReductor> mReductor;
  std::array<std::optional<float>, getNumDE()> mRefValues;
  // Trend plots
  std::unique_ptr<TrendGraph> mOrbits;
  std::array<std::unique_ptr<TrendGraph>, getNumDE()> mTrendsDE;
  std::array<std::unique_ptr<TrendGraph>, getNumDE()> mTrendsRefRatioDE;
  std::array<std::unique_ptr<TrendGraph>, 10> mTrendsChamber;
  std::unique_ptr<TrendMultiGraph> mTrends;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_RATESTRENDSPLOTTER_H
