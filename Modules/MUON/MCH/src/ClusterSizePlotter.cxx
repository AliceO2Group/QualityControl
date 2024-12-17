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
/// \file   ClusterSizePlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/ClusterSizePlotter.h"
#include "MCH/Helpers.h"
#include "MCHMappingInterface/Segmentation.h"
#include <TF1.h>
#include <fmt/format.h>

using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

ClusterSizePlotter::ClusterSizePlotter(std::string path, bool fullPlots) : mPath(path)
{
  mClusterSizeReductor = std::make_unique<ClusterSizeReductor>();

  std::string sc[3] = { "B", "NB", "" };
  std::string sc2[3] = { " (B)", " (NB)", "" };
  int histColors[3] = { kRed, kBlue, kBlack };
  for (int ci = 0; ci < 3; ci++) {

    //----------------------------------
    // Mean cluster size histograms
    //----------------------------------

    mHistogramClusterSizePerDE[ci] = std::make_unique<TH1F>(TString::Format("%sMeanClusterSize%s", mPath.c_str(), sc[ci].c_str()),
                                                            TString::Format("Cluster Size vs DE%s", sc2[ci].c_str()), getNumDE(), 0, getNumDE());
    addHisto(mHistogramClusterSizePerDE[ci].get(), false, "histo", "");

    for (auto de : o2::mch::constants::deIdsForAllMCH) {
      int deID = getDEindex(de);
      if (deID < 0) {
        continue;
      }
      int histId = deID * 3 + ci;
      mHistogramClusterSize[histId] = std::make_unique<TH1F>(TString::Format("%s%sClusterSize_%03d%sHist", mPath.c_str(), getHistoPath(de).c_str(), de, sc[ci].c_str()),
                                                             TString::Format("Cluster Size (DE%03d%s)", de, sc[ci].c_str()), 100, 0, 100);
      mHistogramClusterSize[histId]->SetLineColor(histColors[ci]);
    }
  }

  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    int deID = getDEindex(de);
    if (deID < 0) {
      continue;
    }
    int histId = deID * 3;
    mCanvasClusterSize[deID] = std::make_unique<TCanvas>(TString::Format("%s%sClusterSize_%03d", mPath.c_str(), getHistoPath(de).c_str(), de),
                                                         TString::Format("Cluster Size (DE%03d)", de), 800, 600);
    if (fullPlots) {
      addCanvas(mCanvasClusterSize[deID].get(), mHistogramClusterSize[histId + 2].get(), false, "", "");
    }

    mLegendClusterSize[deID] = std::make_unique<TLegend>(0.7, 0.6, 0.88, 0.88);
    mLegendClusterSize[deID]->SetBorderSize(0);
    mLegendClusterSize[deID]->AddEntry(mHistogramClusterSize[histId].get(), "bending", "l");
    mLegendClusterSize[deID]->AddEntry(mHistogramClusterSize[histId + 1].get(), "non-bending", "l");
    mLegendClusterSize[deID]->AddEntry(mHistogramClusterSize[histId + 2].get(), "full", "l");
  }
}

//_________________________________________________________________________________________

void ClusterSizePlotter::addHisto(TH1* h, bool statBox, const char* drawOptions, const char* displayHints)
{
  h->SetOption(drawOptions);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ h, drawOptions, displayHints });
}

//_________________________________________________________________________________________

void ClusterSizePlotter::addCanvas(TCanvas* c, TH1* h, bool statBox, const char* drawOptions, const char* displayHints)
{
  h->SetOption(drawOptions);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ c, "", displayHints });
}

//_________________________________________________________________________________________

void ClusterSizePlotter::fillHistograms(TH2F* hSize)
{
  // extract the MPVs of the cluster charge distributions
  mClusterSizeReductor->update(hSize);

  for (int ci = 0; ci < 3; ci++) {
    for (size_t de = 0; de < mHistogramClusterSizePerDE[ci]->GetXaxis()->GetNbins(); de++) {
      mHistogramClusterSizePerDE[ci]->SetBinContent(de + 1, mClusterSizeReductor->getDeValue(de, ci));
      mHistogramClusterSizePerDE[ci]->SetBinError(de + 1, 0.1);
    }
  }

  for (int xbin = 1; xbin <= hSize->GetXaxis()->GetNbins(); xbin++) {
    if (xbin > getNumDE() * 3) {
      break;
    }
    TH1F* proj = (TH1F*)hSize->ProjectionY("_proj", xbin, xbin);
    int deId = (xbin - 1) / 3;
    int cId = (xbin - 1) % 3;
    int histId = deId * 3 + cId;
    mHistogramClusterSize[histId]->Reset();
    mHistogramClusterSize[histId]->Add(proj);
    delete proj;
  }

  for (int i = 0; i < mCanvasClusterSize.size(); i++) {
    int histId = i * 3;
    double max1 = mHistogramClusterSize[histId]->GetMaximum();
    double max2 = mHistogramClusterSize[histId + 1]->GetMaximum();
    double max3 = mHistogramClusterSize[histId + 2]->GetMaximum();
    double max = std::max(max1, std::max(max2, max3));
    mCanvasClusterSize[i]->Clear();
    mCanvasClusterSize[i]->cd();
    mHistogramClusterSize[histId + 2]->Draw();
    mHistogramClusterSize[histId + 2]->SetMaximum(max * 1.05);
    mHistogramClusterSize[histId]->Draw("histsame");
    mHistogramClusterSize[histId + 1]->Draw("histsame");
    mLegendClusterSize[i]->Draw();
  }
}

//_________________________________________________________________________________________

void ClusterSizePlotter::update(TH2F* hSize)
{
  fillHistograms(hSize);
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
