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
/// \file   TRDHelpers.cxx
///

#include <TH1.h>
#include <TH2.h>
#include <TLine.h>
#include "TRD/TRDHelpers.h"
#include "DataFormatsTRD/HelperMethods.h"

using namespace o2::trd::constants;
using Helper = o2::trd::HelperMethods;

namespace o2::quality_control_modules::trd
{
bool TRDHelpers::isHalfChamberMasked(int hcid, const std::array<int, MAXCHAMBER>* ptrChamber)
{
  // LB: defined as array in case other chamber status values should be set as not masked
  int GoodStatus[] = { mConfiguredChamberStatus };
  int hcstat = (*ptrChamber)[hcid / 2];
  return (std::find(std::begin(GoodStatus), std::end(GoodStatus), hcstat) == std::end(GoodStatus));
}

void TRDHelpers::drawChamberStatusOnMapAndLayers(const std::array<int, MAXCHAMBER>* ptrChamber, std::shared_ptr<TH2F> hcdist, std::array<std::shared_ptr<TH2F>, NLAYER> mLayers, int unitspersection)
{
  std::pair<float, float> x, y;
  for (int hcid = 0; hcid < MAXCHAMBER * 2; ++hcid) {
    if (isHalfChamberMasked(hcid, ptrChamber)) {
      // Chamber properties
      int hcstat = (*ptrChamber)[hcid / 2];
      int side = hcid % 2;
      int sec = hcid / NHCPERSEC;
      int stack = Helper::getStack(hcid / 2);
      int layer = Helper::getLayer(hcid / 2);
      
      if (hcdist) {
        // Coordinates for half chamber distribution
        int stackLayer = stack * NLAYER + layer;
        int sectorSide = sec * 2 + side;
        x.first = sectorSide;
        x.second = sectorSide + 1;
        y.first = stackLayer;
        y.second = stackLayer + 1;

        drawChamberStatusMask(hcstat, x, y, hcdist);
      }
      
      if (mLayers[layer]) { 
        // Chamber stack of type C0 or C1 condition
        int rowstart = FIRSTROW[stack];
        int rowend = stack == 2 ? rowstart + NROWC0 : rowstart + NROWC1;
        
        // Coordinates for layer map	
        x.first = rowstart - 0.5;
        x.second = rowend - 0.5;
        y.first = (sec * 2 + side) * (unitspersection / 2) - 0.5;
        y.second = y.first + (unitspersection / 2);
        
        drawChamberStatusMask(hcstat, x, y, mLayers[layer]);
      }
    }
  }
}

void TRDHelpers::drawChamberStatusMask(int hcstat, std::pair<float, float> x, std::pair<float, float> y, std::shared_ptr<TH2F> hist)
{
  TLine* box[6];
  box[0] = new TLine(x.first, y.first, x.second, y.first); // bottom
  box[1] = new TLine(x.first, y.second, x.second, y.second); // top
  box[2] = new TLine(x.first, y.first, x.first, y.second); // left
  box[3] = new TLine(x.second, y.first, x.second, y.second);     // right
  box[4] = new TLine(x.first, y.second, x.second, y.first);   // backslash
  box[5] = new TLine(x.first, y.first, x.second, y.second);   // forwardslash
  
  for (int line = 0; line < 6; ++line) {
    if (hcstat == mEmptyChamberStatus) {
      box[line]->SetLineColor(kGray + 1);
    } else if (hcstat == mErrorChamberStatus) {
      box[line]->SetLineColor(kRed);
    } else {
      box[line]->SetLineColor(kBlack);
    }
    box[line]->SetLineWidth(2);
    hist->GetListOfFunctions()->Add(box[line]);
  }
}

void TRDHelpers::drawTrdLayersGrid(TH2F* hist, int unitspersection)
{
  TLine* line;
  for (int i = 0; i < 5; ++i) {
    switch (i) {
      case 0:
        line = new TLine(15.5, 0, 15.5, 18 * unitspersection - 0.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 1:
        line = new TLine(31.5, 0, 31.5, 18 * unitspersection - 0.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 2:
        line = new TLine(43.5, 0, 43.5, 18 * unitspersection - 0.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
      case 3:
        line = new TLine(59.5, 0, 59.5, 18 * unitspersection - 0.5);
        hist->GetListOfFunctions()->Add(line);
        line->SetLineStyle(kDashed);
        line->SetLineColor(kBlack);
        break;
    }
  }

  for (int iSec = 1; iSec < 18; ++iSec) {
    float yPos = iSec * unitspersection - 0.5;
    line = new TLine(-0.5, yPos, 75.5, yPos);
    line->SetLineStyle(kDashed);
    line->SetLineColor(kBlack);
    hist->GetListOfFunctions()->Add(line);
  }
}
} // namespace o2::quality_control_modules::trd
