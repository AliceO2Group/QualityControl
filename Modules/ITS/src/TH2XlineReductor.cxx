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
/// \file   TH2XlineReductor.cxx
/// \author Ivan Ravasenga on the model from Piotr Konopka
///

#include <TH2.h>
#include <TMath.h>
#include "ITS/TH2XlineReductor.h"
#include <iostream>

namespace o2::quality_control_modules::its
{

void* TH2XlineReductor::getBranchAddress()
{
  return &mStats;
}

const char* TH2XlineReductor::getBranchLeafList()
{
  return Form("mean[%i]/D:stddev[%i]:entries[%i]:mean_scaled[%i]", NDIM, NDIM, NDIM, NDIM);
}

void TH2XlineReductor::update(TObject* obj)
{
  auto histo = dynamic_cast<TH2*>(obj);

  //initialize arrays
  for (int i = 0; i < NDIM; i++) {
    mStats.mean[i] = -1.;
    mStats.stddev[i] = -1.;
    mStats.entries[i] = -1.;
    mStats.mean_scaled[i] = -1.;
  }
  if (histo) {
    Double_t entriesx = 0.;
    Double_t sum = 0.;
    for (int iy = 1; iy <= histo->GetNbinsY(); iy++) {
      for (int ix = 1; ix <= histo->GetNbinsX(); ix++) {
        if (histo->GetBinContent(ix, iy) > 0.) {
          entriesx++;
        }
        sum += histo->GetBinContent(ix, iy);
      }
      Double_t meanx = sum / entriesx;
      mStats.mean[iy - 1] = meanx;
      mStats.entries[iy - 1] = entriesx;
      mStats.mean_scaled[iy - 1] = meanx * 512. * 1024.;
      sum = 0.;
      for (int ix = 1; ix <= histo->GetNbinsX(); ix++) {
        Double_t binc = histo->GetBinContent(ix, iy);
        if (binc > 0.) {
          sum += (binc - meanx) * (binc - meanx);
        }
      }
      mStats.stddev[iy - 1] = TMath::Sqrt(sum / (entriesx - 1));
      entriesx = 0.;
      sum = 0.;
    } //end loop on y bins
  }   //end if
}

} // namespace o2::quality_control_modules::its
