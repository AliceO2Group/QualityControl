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
/// \file   OrbitsPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/OrbitsPlotter.h"
#include "MCH/Helpers.h"
#include "MCHGlobalMapping/DsIndex.h"
#include <fmt/format.h>

using namespace o2::mch;
using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

OrbitsPlotter::OrbitsPlotter(std::string path)
{
  mElec2DetMapper = createElec2DetMapper<ElectronicMapperGenerated>();
  mFeeLink2SolarMapper = createFeeLink2SolarMapper<ElectronicMapperGenerated>();

  //----------------------------------
  // Orbits histogram
  //----------------------------------
  mHistogramOrbits = std::make_unique<TH2F>(TString::Format("%sDigitOrbitInTFDE", path.c_str()), "Digit orbits vs DE", getNumDE(), 0, getNumDE(), 768, -384, 384);
  addHisto(mHistogramOrbits.get(), false, "colz", "colz");
}

//_________________________________________________________________________________________

void OrbitsPlotter::update(TH2F* h)
{
  if (!h) {
    return;
  }

  auto incrementBin = [&](TH2F* h, float x, float y, float val) {
    int bx = h->GetXaxis()->FindBin(x);
    int by = h->GetYaxis()->FindBin(y);
    auto entries = h->GetBinContent(bx, by);
    h->SetBinContent(bx, by, entries + val);
  };

  mHistogramOrbits->Reset();

  // loop over bins in electronics coordinates, and map the channels to the corresponding cathode pads
  int nbinsx = h->GetXaxis()->GetNbins();
  int nbinsy = h->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    // address of the DS board in detector representation
    auto dsDetId = getDsDetId(i - 1);
    auto deId = dsDetId.deId();

    int deIndex = getDEindex(deId);
    if (deIndex < 0) {
      continue;
    }

    for (int j = 1; j <= nbinsy; j++) {
      float entries = h->GetBinContent(i, j);
      int orbit = h->GetYaxis()->GetBinCenter(j);
      incrementBin(mHistogramOrbits.get(), deIndex, orbit, entries);
    }
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
