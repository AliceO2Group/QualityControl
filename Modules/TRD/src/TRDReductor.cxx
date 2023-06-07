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
/// \file   TRDReductor.cxx
///

#include <TH1.h>
#include <TF1.h>
#include "TRD/TRDReductor.h"

namespace o2::quality_control_modules::trd
{

void* TRDReductor::getBranchAddress()
{
  return &mStats;
}

const char* TRDReductor::getBranchLeafList()
{
  return "t0peak/D:chi2byNDF/D";
}

void TRDReductor::update(TObject* obj)
{
  auto histo = dynamic_cast<TH1*>(obj);
  if (histo) {
    // Defining Fit function
    TF1* f1 = new TF1("landaufit", "((x<2) ? ROOT::Math::erf(x)*[0]:[0]) + [1]*TMath::Landau(x,[2],[3])", 0.0, 6.0);
    f1->SetParameters(100000, 100000, 1.48, 1.09);

    // Fitting Pulse Distribution with defined fit function
    histo->Fit(f1, "", "", 0.0, 4.0);
    mStats.t0peak = f1->GetMaximumX(0.0, 4.0);
    mStats.BinWithMaxContent = histo->GetMaximumBin();
    mStats.chi2byNDF = (f1->GetChisquare()) / (f1->GetNDF());
  }
}

} // namespace o2::quality_control_modules::trd