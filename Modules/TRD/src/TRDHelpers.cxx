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

void TRDHelpers::drawChamberStatusOnHistograms(const std::array<int, MAXCHAMBER>* ptrChamber, std::shared_ptr<TH2F> chamberMap, std::array<std::shared_ptr<TH2F>, NLAYER> ptrLayersArray, int unitsPerSection)
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

      if (chamberMap) {
        // Coordinates for half chamber distribution
        int stackLayer = stack * NLAYER + layer;
        int sectorSide = sec * 2 + side;
        x.first = sectorSide;
        x.second = sectorSide + 1;
        y.first = stackLayer;
        y.second = stackLayer + 1;

        drawHalfChamberMask(hcstat, x, y, chamberMap);
      }

      if (ptrLayersArray[layer]) {
        // Chamber stack of type C0 or C1 condition
        int rowstart = FIRSTROW[stack];
        int rowend = stack == 2 ? rowstart + NROWC0 : rowstart + NROWC1;

        // Coordinates for layer map
        x.first = rowstart - 0.5;
        x.second = rowend - 0.5;
        y.first = (sec * 2 + side) * (unitsPerSection / 2) - 0.5;
        y.second = y.first + (unitsPerSection / 2);

        drawHalfChamberMask(hcstat, x, y, ptrLayersArray[layer]);
      }
    }
  }
}

void TRDHelpers::drawHalfChamberMask(int hcstat, std::pair<float, float> xCoord, std::pair<float, float> yCoord, std::shared_ptr<TH2F> histogram)
{
  TLine* box[6];
  box[0] = new TLine(xCoord.first, yCoord.first, xCoord.second, yCoord.first);   // bottom
  box[1] = new TLine(xCoord.first, yCoord.second, xCoord.second, yCoord.second); // top
  box[2] = new TLine(xCoord.first, yCoord.first, xCoord.first, yCoord.second);   // left
  box[3] = new TLine(xCoord.second, yCoord.first, xCoord.second, yCoord.second); // right
  box[4] = new TLine(xCoord.first, yCoord.second, xCoord.second, yCoord.first);  // backslash
  box[5] = new TLine(xCoord.first, yCoord.first, xCoord.second, yCoord.second);  // forwardslash

  for (int line = 0; line < 6; ++line) {
    if (hcstat == mEmptyChamberStatus) {
      box[line]->SetLineColor(kGray + 1);
    } else if (hcstat == mErrorChamberStatus) {
      box[line]->SetLineColor(kRed);
    } else {
      box[line]->SetLineColor(kBlack);
    }
    box[line]->SetLineWidth(2);
    histogram->GetListOfFunctions()->Add(box[line]);
  }
}

void TRDHelpers::addChamberGridToHistogram(std::shared_ptr<TH2F> histogram, int unitsPerSection)
{
  TLine* line;
  for (int iStack = 0; iStack < NSTACK; ++iStack) {
    line = new TLine(FIRSTROW[iStack] - 0.5, 0, FIRSTROW[iStack] - 0.5, NSECTOR * unitsPerSection - 0.5);
    line->SetLineStyle(kDashed);
    line->SetLineColor(kBlack);
    histogram->GetListOfFunctions()->Add(line);
  }

  for (int iSec = 1; iSec < NSECTOR; ++iSec) {
    float yPos = iSec * unitsPerSection - 0.5;
    line = new TLine(-0.5, yPos, FIRSTROW[NSTACK - 1] + NROWC1 - 0.5, yPos);
    line->SetLineStyle(kDashed);
    line->SetLineColor(kBlack);
    histogram->GetListOfFunctions()->Add(line);
  }
}
} // namespace o2::quality_control_modules::trd
