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
  return "mean/D:stddev:entries:classContentMinBias1:classContentMinBias2:classContentDMC:classContentEMC:classContentPHO:inputContentMinBias1:inputContentMinBias2:inputContentDMC:inputContentEMC:inputContentPHO";
}

void TH1ctpReductor::update(TObject* obj)
{
  // todo: use GetStats() instead?
  auto histo = dynamic_cast<TH1*>(obj);
  if (histo) {
    mStats.entries = histo->GetEntries();
    mStats.stddev = histo->GetStdDev();
    mStats.mean = histo->GetMean();

    mStats.classContentMinBias1 = histo->GetBinContent(mMinBias1ClassIndex);
    mStats.classContentMinBias2 = histo->GetBinContent(mMinBias2ClassIndex);
    mStats.classContentDMC = histo->GetBinContent(mDMCClassIndex);
    mStats.classContentEMC = histo->GetBinContent(mEMCClassIndex);
    mStats.classContentPHO = histo->GetBinContent(mPH0ClassIndex);

    mStats.inputContentMinBias1 = histo->GetBinContent(mMinBias1InputIndex);
    mStats.inputContentMinBias2 = histo->GetBinContent(mMinBias2InputIndex);
    mStats.inputContentDMC = histo->GetBinContent(mDMCInputIndex);
    mStats.inputContentEMC = histo->GetBinContent(mEMCInputIndex);
    mStats.inputContentPHO = histo->GetBinContent(mPHOInputIndex);
  }
}
} // namespace o2::quality_control_modules::ctp
