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
/// \file   ClusterChargeTrendsPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/ClusterChargeTrendsPlotter.h"
#include <TH2F.h>
#include <fmt/format.h>

using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

ClusterChargeTrendsPlotter::ClusterChargeTrendsPlotter(std::string path, TH2F* hRef, bool fullPlots)
{
  mReductor = std::make_unique<ClusterChargeReductor>();

  //--------------------------------------------------
  // Cluster charge reference values
  //--------------------------------------------------

  std::unique_ptr<ClusterChargeReductor> reductorRef;
  if (hRef) {
    reductorRef = std::make_unique<ClusterChargeReductor>();
    reductorRef->update(hRef);
  }

  //--------------------------------------------------
  // Cluster charge trends
  //--------------------------------------------------

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    int deID = getDEindex(de);
    if (deID < 0) {
      continue;
    }

    std::optional<float> refVal;
    if (reductorRef) {
      refVal = reductorRef->getDeValue(deID);
    }

    mTrends[deID] = std::make_unique<TrendGraph>(fmt::format("{}{}/ClusterCharge_DE{}", path, getHistoPath(de), de),
                                                 fmt::format("DE{} Cluster Charge MPV", de), "charge (ADC)", refVal);
    // mTrends[deID]->setRange(0, 1.2);
    if (fullPlots) {
      addCanvas(mTrends[deID].get(), "");
    }
  }
}

//_________________________________________________________________________________________

void ClusterChargeTrendsPlotter::update(long time, TH2F* h)
{
  // extract the integrated average occupancies
  mReductor->update(h);

  for (size_t de = 0; de < getNumDE(); de++) {
    mTrends[de]->update(time, mReductor->getDeValue(de));
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
