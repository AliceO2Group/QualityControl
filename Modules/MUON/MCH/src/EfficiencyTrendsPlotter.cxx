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
/// \file   PhysicsTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include "MCH/EfficiencyTrendsPlotter.h"
#include "MCH/Helpers.h"
#include "MCHMappingInterface/Segmentation.h"
#include <TH2F.h>
#include <fmt/format.h>

using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

EfficiencyTrendsPlotter::EfficiencyTrendsPlotter(std::string path, TH2F* hRef, bool fullPlots)
{
  mElecMapReductor = std::make_unique<TH2ElecMapReductor>();

  std::unique_ptr<TH2ElecMapReductor> reductorRef;
  if (hRef) {
    reductorRef = std::make_unique<TH2ElecMapReductor>();
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

    std::optional<float> refB, refNB;
    if (reductorRef) {
      refB = reductorRef->getDeValue(deID, 0);
      refNB = reductorRef->getDeValue(deID, 1);
    }

    mTrendsEfficiency[deID] = std::make_unique<TrendMultiGraph>(fmt::format("{}{}/Efficiency_DE{}", path, getHistoPath(de), de),
                                                                fmt::format("DE{} Efficiency", de), "efficiency");
    mTrendsEfficiency[deID]->addGraph("B", "bending    ", refB);
    mTrendsEfficiency[deID]->addGraph("NB", "non-bending", refNB);
    mTrendsEfficiency[deID]->addLegends();
    // mTrendsEfficiency[deID]->setRange(0, 1.2);

    if (fullPlots) {
      addCanvas(mTrendsEfficiency[deID].get(), "");
    }
  }
}

//_________________________________________________________________________________________

void EfficiencyTrendsPlotter::update(long time, TH2F* h)
{
  // extract the integrated average occupancies
  mElecMapReductor->update(h);

  for (size_t de = 0; de < getNumDE(); de++) {
    std::array<double, 2> values;
    for (int ci = 0; ci < 2; ci++) {
      values[ci] = mElecMapReductor->getDeValue(de, ci);
    }
    mTrendsEfficiency[de]->update(time, values);
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
