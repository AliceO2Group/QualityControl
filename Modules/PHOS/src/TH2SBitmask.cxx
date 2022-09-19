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
/// \file   TH2SBitmask.cxx
/// \author Dmitri Peresunko
///

#include "PHOS/TH2SBitmask.h"

namespace o2::quality_control_modules::phos
{
void TH2SBitmask::merge(MergeInterface* const other)
{
  // combine two histograms representing bitmasks
  auto otherHisto = dynamic_cast<const TH2SBitmask* const>(other);
  if (otherHisto) {
    for (unsigned int ix = 1; ix <= this->GetNbinsX(); ix++) {
      for (unsigned int iz = 1; iz <= this->GetNbinsY(); iz++) {
        int cont = this->GetBinContent(ix, iz);
        cont |= int(otherHisto->GetBinContent(ix, iz, cont));
      }
    }
  }
}
} // namespace o2::quality_control_modules::phos
