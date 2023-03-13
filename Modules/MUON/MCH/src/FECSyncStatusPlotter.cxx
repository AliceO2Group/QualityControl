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
/// \file   FECSyncStatusPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/FECSyncStatusPlotter.h"
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

static void setYAxisLabels(TH2F* hErrors)
{
  TAxis* ay = hErrors->GetYaxis();
  for (int i = 1; i <= 10; i++) {
    auto label = fmt::format("CH{}", i);
    ay->SetBinLabel(i, label.c_str());
  }
}

FECSyncStatusPlotter::FECSyncStatusPlotter(std::string path)
{
  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();

  //--------------------------------------------
  // Fraction of synchronized boards per DE
  //--------------------------------------------

  mGoodTFFractionPerDE = std::make_unique<TH1F>(TString::Format("%sSyncedBoardsFractionPerDE", path.c_str()), "Synchronized boards fraction", getNumDE(), 0, getNumDE());
  addHisto(mGoodTFFractionPerDE.get(), false, "hist", "");

  mGoodBoardsFractionPerDE = std::make_unique<TH1F>(TString::Format("%sSyncedBoardsFractionPerDEalt", path.c_str()), "Synchronized boards fraction (v.2)", getNumDE(), 0, getNumDE());
  addHisto(mGoodBoardsFractionPerDE.get(), false, "hist", "");
}

//_________________________________________________________________________________________

void FECSyncStatusPlotter::update(TH2F* h)
{
  if (!h) {
    return;
  }

  std::vector<double> deNumBoards(static_cast<size_t>(getNumDE()));
  std::vector<double> deGoodBoards(static_cast<size_t>(getNumDE()));
  std::vector<double> deGoodFrac(static_cast<size_t>(getNumDE()));

  std::fill(deNumBoards.begin(), deNumBoards.end(), 0);
  std::fill(deGoodBoards.begin(), deGoodBoards.end(), 0);
  std::fill(deGoodFrac.begin(), deGoodFrac.end(), 0);

  int nbinsx = h->GetXaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    // address of the DS board in detector representation
    auto dsDetId = getDsDetId(i - 1);
    auto deId = dsDetId.deId();

    int deIndex = getDEindex(deId);
    if (deIndex < 0) {
      continue;
    }

    auto nGood = h->GetBinContent(i, 1);
    auto nBad = h->GetBinContent(i, 2);
    auto nMissing = h->GetBinContent(i, 3);
    auto nTF = nGood + nBad + nMissing;
    auto goodFrac = nGood / nTF;

    deNumBoards[deIndex] += 1;
    if (nGood == nTF) {
      deGoodBoards[deIndex] += 1;
    }
    deGoodFrac[deIndex] += goodFrac;
  }

  // update the average number of out-of-sync boards
  for (size_t i = 0; i < deNumBoards.size(); i++) {
    if (deNumBoards[i] > 0) {
      mGoodBoardsFractionPerDE->SetBinContent(i + 1, deGoodBoards[i] / deNumBoards[i]);
      mGoodTFFractionPerDE->SetBinContent(i + 1, deGoodFrac[i] / deNumBoards[i]);
    } else {
      mGoodBoardsFractionPerDE->SetBinContent(i + 1, 0);
      mGoodTFFractionPerDE->SetBinContent(i + 1, 0);
    }
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
