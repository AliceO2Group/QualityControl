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
/// \file   EfficiencyPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/EfficiencyPlotter.h"
#include "MCH/Helpers.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHGlobalMapping/DsIndex.h"

using namespace o2::mch;
using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

EfficiencyPlotter::EfficiencyPlotter(std::string path, bool fullPlots)
{
  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mDet2ElecMapper = createDet2ElecMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  mElecMapReductor = std::make_unique<TH2ElecMapReductor>();

  //----------------------------------
  // Mean efficiency histograms
  //----------------------------------

  std::string sc[2] = { "B", "NB" };
  for (int ci = 0; ci < 2; ci++) {
    mHistogramMeanEfficiencyPerDE[ci] = std::make_unique<TH1F>(TString::Format("%sMeanEfficiency%s", path.c_str(), sc[ci].c_str()),
                                                               TString::Format("Mean Efficiency vs DE (%s)", sc[ci].c_str()),
                                                               getNumDE(), 0, getNumDE());
    addDEBinLabels(mHistogramMeanEfficiencyPerDE[ci].get());
    addHisto(mHistogramMeanEfficiencyPerDE[ci].get(), false, "histo", "histo");
  }

  mHistogramMeanEfficiencyPerSolar = std::make_unique<TH1F>(TString::Format("%sMeanEfficiencyPerSolar", path.c_str()), "Mean Efficiency per SOLAR board",
                                                            getNumSolar(), 0, getNumSolar());
  addSolarBinLabels(mHistogramMeanEfficiencyPerSolar.get());
  addHisto(mHistogramMeanEfficiencyPerSolar.get(), false, "histo", "histo");

  //--------------------------------------------------
  // Efficiency histograms in global detector coordinates
  //--------------------------------------------------

  mHistogramEfficiencyGlobal[0] = std::make_unique<GlobalHistogram>(fmt::format("{}Efficiency_ST12", path), "ST12 Efficiency", 0, 5);
  mHistogramEfficiencyGlobal[0]->init();
  addHisto(mHistogramEfficiencyGlobal[0]->getHist(), false, "colz", "");

  mHistogramEfficiencyGlobal[1] = std::make_unique<GlobalHistogram>(fmt::format("{}Efficiency_ST345", path), "ST345 Efficiency", 1, 10);
  mHistogramEfficiencyGlobal[1]->init();
  addHisto(mHistogramEfficiencyGlobal[1]->getHist(), false, "colz", "");

  //--------------------------------------------------
  // Efficiency histograms in detector coordinates
  //--------------------------------------------------

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    auto h = std::make_shared<DetectorHistogram>(TString::Format("%s%sEfficiency_XY_B_%03d", path.c_str(), getHistoPath(de).c_str(), de),
                                                 TString::Format("Hit Efficiency (DE%03d B)", de), de, int(0));
    mHistogramEfficiencyDE[0].insert(make_pair(de, h));
    if (fullPlots) {
      addHisto(h->getHist(), false, "colz", "");
    }

    h = std::make_shared<DetectorHistogram>(TString::Format("%s%sEfficiency_XY_NB_%03d", path.c_str(), getHistoPath(de).c_str(), de),
                                            TString::Format("Hit Efficiency (DE%03d NB)", de), de, int(1));
    mHistogramEfficiencyDE[1].insert(make_pair(de, h));
    if (fullPlots) {
      addHisto(h->getHist(), false, "colz", "");
    }
  }
}

//_________________________________________________________________________________________

void EfficiencyPlotter::fillAverageHistograms()
{
  for (int ci = 0; ci < 2; ci++) {
    for (size_t de = 0; de < mHistogramMeanEfficiencyPerDE[ci]->GetXaxis()->GetNbins(); de++) {
      mHistogramMeanEfficiencyPerDE[ci]->SetBinContent(de + 1, mElecMapReductor->getDeValue(de, ci));
      mHistogramMeanEfficiencyPerDE[ci]->SetBinError(de + 1, 0.1);
    }
  }

  for (size_t solar = 0; solar < mHistogramMeanEfficiencyPerSolar->GetXaxis()->GetNbins(); solar++) {
    mHistogramMeanEfficiencyPerSolar->SetBinContent(solar + 1, mElecMapReductor->getSolarValue(solar));
    mHistogramMeanEfficiencyPerSolar->SetBinError(solar + 1, 0.1);
  }
}

//_________________________________________________________________________________________

void EfficiencyPlotter::fillGlobalHistograms(TH2F* h)
{
  if (!h) {
    return;
  }

  // loop over bins in electronics coordinates, and map the channels to the corresponding cathode pads
  int nbinsx = h->GetXaxis()->GetNbins();
  int nbinsy = h->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    // address of the DS board in detector representation
    auto dsDetId = getDsDetId(i - 1);
    auto deId = dsDetId.deId();
    auto dsId = dsDetId.dsId();

    for (int j = 1; j <= nbinsy; j++) {
      int channel = j - 1;
      int padId = -1;

      const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
      padId = segment.findPadByFEE(dsId, int(channel));

      if (padId < 0) {
        continue;
      }

      double eff = h->GetBinContent(i, j);

      double padX = segment.padPositionX(padId);
      double padY = segment.padPositionY(padId);
      float padSizeX = segment.padSizeX(padId);
      float padSizeY = segment.padSizeY(padId);
      int cathode = segment.isBendingPad(padId) ? 0 : 1;

      // Fill 2D rate histograms
      auto hEfficiency = mHistogramEfficiencyDE[cathode].find(deId);
      if ((hEfficiency != mHistogramEfficiencyDE[cathode].end()) && (hEfficiency->second != NULL)) {
        hEfficiency->second->Set(padX, padY, padSizeX, padSizeY, eff);
      }
    }
  }

  mHistogramEfficiencyGlobal[0]->set(mHistogramEfficiencyDE[0], mHistogramEfficiencyDE[1]);
  mHistogramEfficiencyGlobal[1]->set(mHistogramEfficiencyDE[0], mHistogramEfficiencyDE[1]);
}

//_________________________________________________________________________________________

void EfficiencyPlotter::update(TH2F* hEfficiency)
{
  // extract the integrated average occupancies
  mElecMapReductor->update(hEfficiency);

  fillAverageHistograms();
  fillGlobalHistograms(hEfficiency);
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
