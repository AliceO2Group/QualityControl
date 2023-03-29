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
/// \file   HeartBeatPacketsPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/HeartBeatPacketsPlotter.h"
#include "MCH/Helpers.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHGlobalMapping/DsIndex.h"
#include <iostream>
#include <fmt/format.h>

using namespace o2::mch;
using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

HeartBeatPacketsPlotter::HeartBeatPacketsPlotter(std::string path, bool fullPlots)
{
  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();

  //--------------------------------------------------
  // Rates histograms in global detector coordinates
  //--------------------------------------------------

  mHistogramHBRateGlobal[0] = std::make_shared<GlobalHistogram>(fmt::format("{}HBRate_ST12", path), "ST12 HeartBeat Rate", 0, 5);
  mHistogramHBRateGlobal[0]->init();
  addHisto(mHistogramHBRateGlobal[0]->getHist(), false, "colz", "colz");

  mHistogramHBRateGlobal[1] = std::make_shared<GlobalHistogram>(fmt::format("{}HBRate_ST345", path), "ST345 HeartBeat Rate", 1, 10);
  mHistogramHBRateGlobal[1]->init();
  addHisto(mHistogramHBRateGlobal[1]->getHist(), false, "colz", "colz");

  //--------------------------------------------------
  // Rates histograms in detector coordinates
  //--------------------------------------------------

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    auto h = std::make_shared<DetectorHistogram>(TString::Format("%s%sHBRate_XY_B_%03d", path.c_str(), getHistoPath(de).c_str(), de),
                                                 TString::Format("HeartBeat  Rate (DE%03d B)", de), de, int(0));
    mHistogramHBRateDE[0].insert(make_pair(de, h));
    if (fullPlots) {
      addHisto(h->getHist(), false, "colz", "colz");
    }

    h = std::make_shared<DetectorHistogram>(TString::Format("%s%sHBRate_XY_NB_%03d", path.c_str(), getHistoPath(de).c_str(), de),
                                            TString::Format("HeartBeat  Rate (DE%03d NB)", de), de, int(1));
    mHistogramHBRateDE[1].insert(make_pair(de, h));
    if (fullPlots) {
      addHisto(h->getHist(), false, "colz", "colz");
    }
  }
}

//_________________________________________________________________________________________

void HeartBeatPacketsPlotter::update(TH2F* h)
{
  if (!h) {
    return;
  }

  int nbinsx = h->GetXaxis()->GetNbins();
  int nbinsy = h->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    // address of the DS board in detector representation
    auto dsDetId = getDsDetId(i - 1);
    auto deId = dsDetId.deId();
    auto dsId = dsDetId.dsId();

    // total number of HB packets received
    auto nHB = h->Integral(i, i, 1, nbinsy);

    for (int channel = 0; channel < 64; channel++) {
      int padId = -1;

      const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
      padId = segment.findPadByFEE(dsId, int(channel));

      if (padId < 0) {
        continue;
      }

      double padX = segment.padPositionX(padId);
      double padY = segment.padPositionY(padId);
      float padSizeX = segment.padSizeX(padId);
      float padSizeY = segment.padSizeY(padId);
      int cathode = segment.isBendingPad(padId) ? 0 : 1;

      // Fill 2D rate histograms
      auto hRate = mHistogramHBRateDE[cathode].find(deId);
      if ((hRate != mHistogramHBRateDE[cathode].end()) && (hRate->second != NULL)) {
        hRate->second->Set(padX, padY, padSizeX, padSizeY, nHB);
      }
    }
  }

  mHistogramHBRateGlobal[0]->set(mHistogramHBRateDE[0], mHistogramHBRateDE[1]);
  mHistogramHBRateGlobal[1]->set(mHistogramHBRateDE[0], mHistogramHBRateDE[1]);
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
