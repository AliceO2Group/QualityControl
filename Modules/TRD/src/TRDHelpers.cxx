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
  // LB: defined as array in case other chamber status values are set as not masked
  int GoodStatus[] = { mConfiguredChamberStatus };
  int hcstat = (*ptrChamber)[hcid / 2];
  return (std::find(std::begin(GoodStatus), std::end(GoodStatus), hcstat) == std::end(GoodStatus));
}

void TRDHelpers::drawChamberStatusOn2D(std::shared_ptr<TH2F> hist, const std::array<int, MAXCHAMBER>* ptrChamber)
{
  TLine* boxlines[6];
  std::pair<int, int> x, y;
  for (int hcid = 0; hcid < MAXCHAMBER * 2; ++hcid) {
    if (isHalfChamberMasked(hcid, ptrChamber)) {
      int stackLayer = Helper::getStack(hcid / 2) * NLAYER + Helper::getLayer(hcid / 2);
      int sectorSide = (hcid / NHCPERSEC) * 2 + (hcid % 2);
      x.first = sectorSide;
      x.second = sectorSide + 1;
      y.first = stackLayer;
      y.second = stackLayer + 1;

      boxlines[0] = new TLine(x.first, y.first, x.second, y.second);
      boxlines[1] = new TLine(x.second, y.first, x.first, y.second);
      boxlines[2] = new TLine(x.first, y.first, x.second, y.first);
      boxlines[3] = new TLine(x.first, y.second, x.second, y.second);
      boxlines[4] = new TLine(x.first, y.first, x.first, y.second);
      boxlines[5] = new TLine(x.second, y.first, x.second, y.second);

      setBoxChamberStatus((*ptrChamber)[hcid / 2], boxlines, hist);
    }
  }
}

void TRDHelpers::drawChamberStatusOnLayers(std::array<std::shared_ptr<TH2F>, NLAYER> mLayers, const std::array<int, MAXCHAMBER>* ptrChamber, int unitspersection)
{
  for (int iLayer = 0; iLayer < NLAYER; ++iLayer) {
    for (int iSec = 0; iSec < 18; ++iSec) {
      for (int iStack = 0; iStack < 5; ++iStack) {
        int rowMax = (iStack == 2) ? 12 : 16;
        for (int side = 0; side < 2; ++side) {
          int det = iSec * 30 + iStack * 6 + iLayer;
          int hcid = (side == 0) ? det * 2 : det * 2 + 1;
          int rowstart = iStack < 3 ? iStack * 16 : 44 + (iStack - 3) * 16;                 // pad row within whole sector
          int rowend = iStack < 3 ? rowMax + iStack * 16 : rowMax + 44 + (iStack - 3) * 16; // pad row within whole sector
          if (isHalfChamberMasked(hcid, ptrChamber)) {
            int hcstat = (*ptrChamber)[hcid / 2];
            std::cout << "Leonardo hcid / 2 " << hcid / 2 << std::endl;
            std::cout << "Leonardo hcstat " << hcstat << std::endl;
            drawHashOnLayers(iLayer, hcid, hcstat, rowstart, rowend, unitspersection, mLayers);
          }
        }
      }
    }
  }
}

void TRDHelpers::drawHashOnLayers(int layer, int hcid, int hcstat, int rowstart, int rowend, int unitspersection, std::array<std::shared_ptr<TH2F>, NLAYER> mLayers)
{
  // Draw a simple box in with a X on it
  std::pair<float, float> topright, bottomleft; // coordinates of box
  TLine* boxlines[6];
  int det = hcid / 2;
  int side = hcid % 2;
  int sec = hcid / 60;

  bottomleft.first = rowstart - 0.5;
  bottomleft.second = (sec * 2 + side) * (unitspersection / 2) - 0.5;
  topright.first = rowend - 0.5;
  topright.second = (sec * 2 + side + 1) * (unitspersection / 2) - 0.5;

  boxlines[0] = new TLine(bottomleft.first, bottomleft.second, topright.first, bottomleft.second); // bottom
  boxlines[1] = new TLine(bottomleft.first, topright.second, topright.first, topright.second);     // top
  boxlines[2] = new TLine(bottomleft.first, bottomleft.second, bottomleft.first, topright.second); // left
  boxlines[3] = new TLine(topright.first, bottomleft.second, topright.first, topright.second);     // right
  boxlines[4] = new TLine(topright.first, bottomleft.second, bottomleft.first, topright.second);   // backslash
  boxlines[5] = new TLine(bottomleft.first, bottomleft.second, topright.first, topright.second);   // forwardslash

  setBoxChamberStatus(hcstat, boxlines, mLayers[layer]);
}

void TRDHelpers::setBoxChamberStatus(int hcstat, TLine* box[], std::shared_ptr<TH2F> hist)
{
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
