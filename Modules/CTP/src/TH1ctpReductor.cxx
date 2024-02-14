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
/// \file   TH1ctpReductor.cxx
/// \author Lucia Anna Tarasovicova
///

#include <TH1.h>
#include "CTP/TH1ctpReductor.h"

namespace o2::quality_control_modules::ctp
{

void* TH1ctpReductor::getBranchAddress()
{
  return &mStats;
}

const char* TH1ctpReductor::getBranchLeafList()
{
  return Form("mean/D:stddev:entries:inputs[%i]:classContentMTVX:classContentMVBA:classContentTVXDMC:classContentTVXEMC:classContentTVXPHO", nInputs);
}

void TH1ctpReductor::update(TObject* obj)
{
  // todo: use GetStats() instead?
  auto histo = dynamic_cast<TH1*>(obj);
  if (histo) {
    mStats.entries = histo->GetEntries();
    mStats.stddev = histo->GetStdDev();
    mStats.mean = histo->GetMean();
    for (int i = 1; i < nInputs + 1; i++) {
      mStats.inputs[i - 1] = histo->GetBinContent(i);
    }
    mStats.classContentMTVX = histo->GetBinContent(mMTVXIndex);
    mStats.classContentMVBA = histo->GetBinContent(mMVBAIndex);
    mStats.classContentTVXDMC = histo->GetBinContent(mTVXDCMIndex);
    mStats.classContentTVXEMC = histo->GetBinContent(mTVXEMCIndex);
    mStats.classContentTVXPHO = histo->GetBinContent(mTVXPHOIndex);
  }
}
} // namespace o2::quality_control_modules::ctp
