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
/// \file   Helpers.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_EFFICIENCYTRENDSPLOTTER_H
#define QC_MODULE_EFFICIENCYTRENDSPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCH/Helpers.h"
#include "MCH/TH2ElecMapReductor.h"

class TH2F;

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class EfficiencyTrendsPlotter : public HistPlotter
{
 public:
  EfficiencyTrendsPlotter(std::string path, TH2F* hRef, bool fullPlots = false);

  void update(long time, TH2F* hEfficiency);

 private:
  void addCanvas(TCanvas* c, const char* displayHints)
  {
    histograms().emplace_back(HistInfo{ c, "", displayHints });
  }

  // Data reductor
  std::unique_ptr<TH2ElecMapReductor> mElecMapReductor;
  // Trend plots
  std::array<std::unique_ptr<TrendMultiGraph>, getNumDE()> mTrendsEfficiency;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_EFFICIENCYTRENDSPLOTTER_H
