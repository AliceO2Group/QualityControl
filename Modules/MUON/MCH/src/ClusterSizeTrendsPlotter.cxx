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
/// \file   ClusterSizeTrendsPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/ClusterSizeTrendsPlotter.h"
#include <TH2F.h>
#include <fmt/format.h>

using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

ClusterSizeTrendsPlotter::ClusterSizeTrendsPlotter(std::string path, TH2F* hRef, bool fullPlots)
{
  mReductor = std::make_unique<ClusterSizeReductor>();

  std::unique_ptr<ClusterSizeReductor> reductorRef;
  if (hRef) {
    reductorRef = std::make_unique<ClusterSizeReductor>();
    reductorRef->update(hRef);
  }

  //--------------------------------------------------
  // Efficiency trends
  //--------------------------------------------------

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    int deID = getDEindex(de);
    if (deID < 0) {
      continue;
    }

    std::optional<float> refB, refNB, refBNB;
    if (reductorRef) {
      refB = reductorRef->getDeValue(deID, 0);
      refNB = reductorRef->getDeValue(deID, 1);
      refBNB = reductorRef->getDeValue(deID, 2);
    }

    mTrends[deID] = std::make_unique<TrendMultiGraph>(fmt::format("{}{}/ClusterSize_DE{}", path, getHistoPath(de), de),
                                                      fmt::format("DE{} Cluster Size", de), "cluster size");
    mTrends[deID]->addGraph("B", "bending    ", refB);
    mTrends[deID]->addGraph("NB", "non-bending", refNB);
    mTrends[deID]->addGraph("BNB", "full       ", refBNB);
    mTrends[deID]->addLegends();

    if (fullPlots) {
      addCanvas(mTrends[deID].get(), "");
    }
  }
}

//_________________________________________________________________________________________

void ClusterSizeTrendsPlotter::update(long time, TH2F* h)
{
  // extract the integrated average occupancies
  mReductor->update(h);

  for (size_t de = 0; de < getNumDE(); de++) {
    std::array<double, 3> values;
    for (int ci = 0; ci < 3; ci++) {
      values[ci] = mReductor->getDeValue(de, ci);
    }
    mTrends[de]->update(time, values);
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
