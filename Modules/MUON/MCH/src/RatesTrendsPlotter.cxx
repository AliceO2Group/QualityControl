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
/// \file   RatesTrendsPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/RatesTrendsPlotter.h"
#include <TH2F.h>
#include <fmt/format.h>

using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

RatesTrendsPlotter::RatesTrendsPlotter(std::string path, bool fullPlots) : mPath(path)
{
  mReductor = std::make_unique<TH2ElecMapReductor>();


  mOrbits = std::make_unique<TrendGraph>(fmt::format("{}Orbits", path), "Orbits", "orbits");
  addCanvas(mOrbits.get(), "");

  //--------------------------------------------------
  // Rates trends
  //--------------------------------------------------

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    int deID = getDEindex(de);
    if (deID < 0) {
      continue;
    }
    mTrendsDE[deID] = std::make_unique<TrendGraph>(fmt::format("{}{}/Rates_DE{}", path, getHistoPath(de), de),
                                                   fmt::format("DE{} Rates", de), "rate (kHz)");
    if (fullPlots) {
      addCanvas(mTrendsDE[deID].get(), "");
    }
  }

  mTrends = std::make_unique<TrendMultiGraph>(fmt::format("{}ChamberRates", path), "Chamber Rates", "rate (kHz)");
  for (int i = 0; i < mTrendsChamber.size(); i++) {
    int st = (i / 2) + 1;
    int ch = i + 1;
    mTrendsChamber[i] = std::make_unique<TrendGraph>(fmt::format("{}ST{}/Rates_CH{}", path, st, ch),
                                                     fmt::format("CH{} Rates", ch), "rate (kHz)");
    addCanvas(mTrendsChamber[i].get(), "");

    mTrends->addGraph(fmt::format("CH{}", ch), fmt::format("CH{} Rates", ch));
  }
  mTrends->addLegends();
  // mTrends->setRange(0, 1.2);
  addCanvas(mTrends.get(), "");
}

//_________________________________________________________________________________________

void RatesTrendsPlotter::update(long time, TH2F* h)
{
  // extract the integrated average occupancies
  mReductor->update(h);

  mOrbits->update(time, mReductor->getOrbits());

  for (size_t de = 0; de < getNumDE(); de++) {
    mTrendsDE[de]->update(time, mReductor->getDeValue(de));
  }

  std::array<double, 10> values;
  for (size_t ch = 0; ch < 10; ch++) {
    auto value = mReductor->getChamberValue(ch);
    values[ch] = value;
    mTrendsChamber[ch]->update(time, value);
  }
  mTrends->update(time, values);
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
