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
/// \file   ClusterChargeTrendsPlotter.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_CLUSTERCHARGETRENDSPLOTTER_H
#define QC_MODULE_CLUSTERCHARGETRENDSPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCH/Helpers.h"
#include "MCH/ClusterChargeReductor.h"

class TH2F;

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class ClusterChargeTrendsPlotter : public HistPlotter
{
 public:
  ClusterChargeTrendsPlotter(std::string path, TH2F* hRef, bool fullPlots = false);

  void update(long time, TH2F* hEfficiency);

 private:
  void addCanvas(TCanvas* c, const char* displayHints)
  {
    histograms().emplace_back(HistInfo{ c, "", displayHints });
  }

  // Data reductor
  std::unique_ptr<ClusterChargeReductor> mReductor;
  // Trend plots
  std::array<std::unique_ptr<TrendGraph>, getNumDE()> mTrends;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_CLUSTERCHARGETRENDSPLOTTER_H
