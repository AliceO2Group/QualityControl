// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TH1Reductor.cxx
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///

#include <TH1.h>
#include <vector>
#include <TCanvas.h>
#include <TList.h>
#include <TAxis.h>
#include "TPC/TH1ReductorTPC.h"

namespace o2::quality_control_modules::tpc
{

void* TH1ReductorTPC::getBranchAddress()
{
  return &mStats;
}

const char* TH1ReductorTPC::getBranchLeafList()
{
  return Form("mean[%i]/D:stddev[%i]:entries[%i]", NMAXSLICES, NMAXSLICES, NMAXSLICES);
}

void TH1ReductorTPC::update(TObject* obj, std::vector<std::vector<float>>& axis, bool isCanvas)
{
  TList* padList = nullptr;                // List of TPads for input TCanvas.
  int nPads = 1;                           // Number of pads in obj (Set to 1 for TH1).
  TH1* histo = nullptr;                    // General pointer to the histogram to trend. Common for input canvas and slicer.
  int innerAxisSize = (int)axis[0].size(); // Dimension of the inner axisDivision. Set to 1 if no slicing or if input canvas.

  // Get the list of pads/histograms if an input canvas is passed.
  if (isCanvas) {
    auto canvas = dynamic_cast<TCanvas*>(obj);
    padList = dynamic_cast<TList*>(canvas->GetListOfPrimitives());
    nPads = padList->GetEntries();
    innerAxisSize = 1; // Enforce the absence of slicing.
  }

  // Get the trending quantities for each slice/histogram.
  for (int iPad = 0; iPad < nPads; iPad++) {
    if (isCanvas) {
      auto pad = dynamic_cast<TPad*>(padList->At(iPad));
      histo = dynamic_cast<TH1*>(pad->GetListOfPrimitives()->At(0));
    } else { // The passed "obj" is directly an histogram.
      histo = dynamic_cast<TH1*>(obj);
    }

    // Get the trending quantities depending on the config.
    if (histo) {
      if (innerAxisSize == 1) { // No slicing/input canvas.
        mStats.entries[iPad] = histo->GetEntries();
        mStats.stddev[iPad] = histo->GetStdDev();
        mStats.mean[iPad] = histo->GetMean();

      } else if ((int)axis.size() == 1) {
        for (int i = 0; i < (int)axis.size(); i++) {
          for (int j = 0; j < (int)axis[i].size() - 1; j++) {
            histo->GetXaxis()->SetRangeUser(axis[i][j], axis[i][j + 1]);
            mStats.entries[j] = histo->GetEntries();
            mStats.stddev[j] = histo->GetStdDev();
            mStats.mean[j] = histo->GetMean();
          }
        }
      } else {
        printf("Configuration not supported yet.\n");
      }
    } // if (histo).
  }
}

} // namespace o2::quality_control_modules::tpc
