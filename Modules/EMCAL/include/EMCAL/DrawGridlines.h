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

#ifndef QUALITYCONTROL_DRAWGRIDLINES_H
#define QUALITYCONTROL_DRAWGRIDLINES_H

#include "TLine.h"

namespace o2::quality_control_modules::emcal
{

/// \file  DrawGridlines.h
/// \brief Quality Control helper task for drawing the EMCAL gridlines
/// \author Sierra Cantway

class DrawGridlines final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  DrawGridlines() = default;
  /// \brief Destructor
  ~DrawGridlines() = default;

  /// \brief Draw the gridlines for the Supermodule limits in the Trigger Geometry
  static void DrawSMGridInTriggerGeo()
  {

    // EMCAL
    for (int iside = 0; iside <= 48; iside += 24) {
      auto smline = new TLine(static_cast<double>(iside) - 0.5, -0.5, static_cast<double>(iside) - 0.5, 63.5);
      smline->SetLineWidth(6);
      smline->Draw("same");
    }
    for (int iphi = 0; iphi < 60; iphi += 12) {
      auto smline = new TLine(-0.5, static_cast<double>(iphi) - 0.5, 47.5, static_cast<double>(iphi) - 0.5);
      smline->SetLineWidth(6);
      smline->Draw("same");
    }
    for (auto iphi = 60; iphi <= 64; iphi += 4) {
      auto smline = new TLine(-0.5, static_cast<double>(iphi) - 0.5, 47.5, static_cast<double>(iphi) - 0.5);
      smline->SetLineWidth(6);
      smline->Draw("same");
    }

    // DCAL
    for (int side = 0; side < 2; side++) {
      int sideoffset = (side == 0) ? 0 : 32;
      for (int isepeta = 0; isepeta < 2; isepeta++) {
        int etaoffset = sideoffset + isepeta * 16;
        auto smline = new TLine(static_cast<double>(etaoffset) - 0.5, 63.5, static_cast<double>(etaoffset) - 0.5, 99.5);
        smline->SetLineWidth(6);
        smline->Draw("same");
      }
      for (auto iphi = 76; iphi <= 88; iphi += 12) {
        auto smline = new TLine(static_cast<double>(sideoffset) - 0.5, static_cast<double>(iphi) - 0.5, static_cast<double>(sideoffset + 16) - 0.5, static_cast<double>(iphi) - 0.5);
        smline->SetLineWidth(6);
        smline->Draw("same");
      }
    }
    for (auto iphi = 100; iphi <= 104; iphi += 4) {
      auto smline = new TLine(-0.5, static_cast<double>(iphi) - 0.5, 47.5, static_cast<int>(iphi) - 0.5);
      smline->SetLineWidth(6);
      smline->Draw("same");
    }
    for (auto ieta = 0; ieta <= 48; ieta += 24) {
      auto smline = new TLine(static_cast<double>(ieta) - 0.5, 99.5, static_cast<double>(ieta) - 0.5, 103.5);
      smline->SetLineWidth(6);
      smline->Draw("same");
    }
  };

  /// \brief Draw the gridlines for the TRU limits
  static void DrawTRUGrid()
  {

    // EMCAL
    for (int side = 0; side < 2; side++) {
      int sideoffset = 24 * side;
      for (int itru = 0; itru < 2; itru++) {
        int truoffset = sideoffset + (itru + 1) * 8;
        auto truline = new TLine(static_cast<int>(truoffset) - 0.5, -0.5, static_cast<int>(truoffset) - 0.5, 59.5);
        truline->SetLineWidth(3);
        truline->Draw("same");
      }
    }

    // DCAL
    for (int side = 0; side < 2; side++) {
      int sideoffset = (side == 0) ? 0 : 32;
      auto truseparator = new TLine(static_cast<double>(sideoffset + 8) - 0.5, 63.5, static_cast<double>(sideoffset + 8) - 0.5, 99.5);
      truseparator->SetLineWidth(3);
      truseparator->Draw("same");
    }
  };

  /// \brief Draw the gridlines for the FastOR limits
  static void DrawFastORGrid()
  {

    // EMCAL
    for (int iphi = 1; iphi < 64; iphi++) {
      auto fastorLine = new TLine(-0.5, static_cast<double>(iphi) - 0.5, 47.5, static_cast<double>(iphi) - 0.5);
      fastorLine->Draw("same");
    }
    for (int ieta = 1; ieta < 48; ieta++) {
      auto fastorLine = new TLine(static_cast<double>(ieta) - 0.5, -0.5, static_cast<double>(ieta) - 0.5, 63.5);
      fastorLine->Draw("same");
    }

    // DCAL
    for (int side = 0; side < 2; side++) {
      int sideoffset = (side == 0) ? 0 : 32;
      for (int ieta = 0; ieta <= 16; ieta++) {
        int etaoffset = sideoffset + ieta;
        auto fastorline = new TLine(static_cast<double>(etaoffset - 0.5), 63.5, static_cast<double>(etaoffset) - 0.5, 99.5);
        fastorline->Draw("same");
      }
      for (int iphi = 0; iphi <= 36; iphi++) {
        int phioffset = iphi + 64;
        auto fastorline = new TLine(static_cast<double>(sideoffset - 0.5), static_cast<double>(phioffset - 0.5), static_cast<double>(sideoffset + 16) - 0.5, static_cast<double>(phioffset) - 0.5);
        fastorline->Draw("same");
      }
    }
    for (auto ieta = 1; ieta < 48; ieta++) {
      auto etaline = new TLine(static_cast<double>(ieta) - 0.5, 99.5, static_cast<double>(ieta) - 0.5, 103.5);
      etaline->Draw("same");
    }
    for (auto iphi = 101; iphi <= 103; iphi++) {
      auto philine = new TLine(-0.5, static_cast<double>(iphi) - 0.5, 47.5, static_cast<int>(iphi) - 0.5);
      philine->Draw("same");
    }
  };
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_DRAWGRIDLINES_H
