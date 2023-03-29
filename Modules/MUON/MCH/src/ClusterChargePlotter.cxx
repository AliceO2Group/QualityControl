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
/// \file   ClusterChargePlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/ClusterChargePlotter.h"

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

ClusterChargePlotter::ClusterChargePlotter(std::string path, TH2F* hRef, bool fullPlots)
{
  mChargeReductor = std::make_unique<ClusterChargeReductor>();

  //----------------------------------
  // Reference charge MPV histogram
  //----------------------------------
  mHistogramChargePerDERef =
    std::make_unique<TH1F>(TString::Format("%sChargeRef", path.c_str()),
                           TString::Format("Charge vs DE, reference"), getNumDE(), 0, getNumDE());
  mHistogramChargePerDERef->SetLineColor(kRed);
  mHistogramChargePerDERef->SetLineStyle(kDashed);
  mHistogramChargePerDERef->SetLineWidth(2);

  if (hRef) {
    ClusterChargeReductor reductorRef;
    reductorRef.update(hRef);

    for (size_t de = 0; de < mHistogramChargePerDERef->GetXaxis()->GetNbins(); de++) {
      mHistogramChargePerDERef->SetBinContent(de + 1, reductorRef.getDeValue(de));
      mHistogramChargePerDERef->SetBinError(de + 1, 0);
    }
  }

  //----------------------------------
  // Charge MPV histograms
  //----------------------------------

  mHistogramChargePerDE = std::make_unique<TH1F>(TString::Format("%sClusterChargeMPVHist", path.c_str()),
                                                 TString::Format("Charge vs DE"), getNumDE(), 0, getNumDE());

  mHistogramChargeRefRatio = std::make_unique<TH1F>(TString::Format("%sClusterChargeMPVRefRatio", path.c_str()),
                                                    TString::Format("Charge vs DE, ratio wrt reference"), getNumDE(), 0, getNumDE());
  addHisto(mHistogramChargeRefRatio.get(), false, "histo", "");

  mCanvasChargePerDE = std::make_unique<TCanvas>(TString::Format("%sClusterChargeMPV", path.c_str()),
                                                 TString::Format("Charge MPV vs DE"), 800, 600);
  // mCanvasChargePerDE->SetLogy(kTRUE);
  addCanvas(mCanvasChargePerDE.get(), mHistogramChargePerDE.get(), false, "", "");

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    int deID = getDEindex(de);
    if (deID < 0) {
      continue;
    }
    mHistogramCharge[deID] = std::make_unique<TH1F>(TString::Format("%s%sClusterCharge_%03d", path.c_str(), getHistoPath(de).c_str(), de),
                                                    TString::Format("Cluster Charge (DE%03d)", de), 256, 0, 256 * 50);
    if (fullPlots) {
      addHisto(mHistogramCharge[deID].get(), false, "hist", "");
    }
  }
}

//_________________________________________________________________________________________

void ClusterChargePlotter::update(TH2F* hCharge)
{
  // extract the MPVs of the cluster charge distributions
  mChargeReductor->update(hCharge);

  for (size_t de = 0; de < mHistogramChargePerDE->GetXaxis()->GetNbins(); de++) {
    mHistogramChargePerDE->SetBinContent(de + 1, mChargeReductor->getDeValue(de));
    mHistogramChargePerDE->SetBinError(de + 1, 0.1);
  }

  mCanvasChargePerDE->Clear();
  mCanvasChargePerDE->cd();
  mHistogramChargePerDE->Draw();

  if (mHistogramChargePerDERef) {
    mHistogramChargePerDERef->Draw("histsame");

    mHistogramChargeRefRatio->Reset();
    mHistogramChargeRefRatio->Add(mHistogramChargePerDE.get());
    mHistogramChargeRefRatio->Divide(mHistogramChargePerDERef.get());

    // special handling of bins with zero rate in reference
    int nbinsx = mHistogramChargePerDERef->GetXaxis()->GetNbins();
    for (int b = 1; b <= nbinsx; b++) {
      if (mHistogramChargePerDERef->GetBinContent(b) == 0) {
        mHistogramChargeRefRatio->SetBinContent(b, 1);
        mHistogramChargeRefRatio->SetBinError(b, 0);
      }
    }
  }

  for (int xbin = 1; xbin <= hCharge->GetXaxis()->GetNbins(); xbin++) {
    if (xbin > getNumDE()) {
      break;
    }
    TH1F* proj = (TH1F*)hCharge->ProjectionY("_proj", xbin, xbin);
    int de = xbin - 1;
    mHistogramCharge[de]->Reset();
    mHistogramCharge[de]->Add(proj);
    delete proj;
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
