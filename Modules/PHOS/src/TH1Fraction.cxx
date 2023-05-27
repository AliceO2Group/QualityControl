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
/// \file   TH1Fraction.cxx
/// \author Sergey Evdokimov
///

#include "PHOS/TH1Fraction.h"

namespace o2::quality_control_modules::phos
{
TH1Fraction::TH1Fraction(const char* name, const char* title, Int_t nbinsx, Double_t xlow, Double_t xup) : TH1D(name, title, nbinsx, xlow, xup)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mUnderlyingCounts = new TH1D(Form("%s_underlying", name), title, nbinsx, xlow, xup);
  TH1::AddDirectory(bStatus);

  if (!mUnderlyingCounts) {
    return;
  }
  mUnderlyingCounts->Sumw2();
}

TH1Fraction::TH1Fraction(TH1Fraction const& copymerge) : TH1D(copymerge.GetName(), copymerge.GetTitle(),
                                                              copymerge.GetXaxis()->GetNbins(),
                                                              copymerge.GetXaxis()->GetXmin(),
                                                              copymerge.GetXaxis()->GetXmax()),
                                                         o2::mergers::MergeInterface(),
                                                         mEventCounter(copymerge.getEventCounter())
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mUnderlyingCounts = (TH1D*)copymerge.getUnderlyingCounts()->Clone();
  TH1::AddDirectory(bStatus);

  if (!mUnderlyingCounts) {
    return;
  }
  mUnderlyingCounts->Sumw2();
  update();
}

void TH1Fraction::update()
{
  if (mEventCounter) {
    for (int i = 1; i <= GetXaxis()->GetNbins(); i++) {
      SetBinContent(i, mUnderlyingCounts->GetBinContent(i) / mEventCounter);
      SetBinError(i, mUnderlyingCounts->GetBinError(i) / mEventCounter);
    }
  }
}

void TH1Fraction::merge(MergeInterface* const other)
{
  // special merge method to approximately combine two histograms
  auto otherHisto = dynamic_cast<const TH1Fraction* const>(other);
  if (otherHisto) {
    if (mUnderlyingCounts->Add(otherHisto->getUnderlyingCounts())) {
      mEventCounter += otherHisto->getEventCounter();
    }
  }
  update();
}

void TH1Fraction::Reset(Option_t* option)
{
  if (mUnderlyingCounts) {
    mUnderlyingCounts->Reset(option);
  }
  mEventCounter = 0;
  TH1D::Reset(option);
}

} // namespace o2::quality_control_modules::phos
