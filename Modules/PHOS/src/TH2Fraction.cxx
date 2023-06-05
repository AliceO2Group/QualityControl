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
/// \file   TH2Fraction.cxx
/// \author Sergey Evdokimov
///

#include "PHOS/TH2Fraction.h"

namespace o2::quality_control_modules::phos
{
TH2Fraction::TH2Fraction(const char* name, const char* title, Int_t nbinsx, Double_t xlow, Double_t xup,
                         Int_t nbinsy, Double_t ylow, Double_t yup) : TH2D(name, title, nbinsx, xlow, xup, nbinsy, ylow, yup)
{
  Bool_t bStatus = TH2::AddDirectoryStatus();
  TH2::AddDirectory(kFALSE);
  mUnderlyingCounts = new TH2D(Form("%s_underlying", name), title, nbinsx, xlow, xup, nbinsy, ylow, yup);
  TH2::AddDirectory(bStatus);

  if (!mUnderlyingCounts) {
    return;
  }
  mUnderlyingCounts->Sumw2();
}

TH2Fraction::TH2Fraction(TH2Fraction const& copymerge) : TH2D(copymerge.GetName(), copymerge.GetTitle(),
                                                              copymerge.GetXaxis()->GetNbins(),
                                                              copymerge.GetXaxis()->GetXmin(),
                                                              copymerge.GetXaxis()->GetXmax(),
                                                              copymerge.GetYaxis()->GetNbins(),
                                                              copymerge.GetYaxis()->GetXmin(),
                                                              copymerge.GetYaxis()->GetXmax()),
                                                         o2::mergers::MergeInterface(),
                                                         mEventCounter(copymerge.getEventCounter())
{
  Bool_t bStatus = TH2::AddDirectoryStatus();
  TH2::AddDirectory(kFALSE);
  mUnderlyingCounts = (TH2D*)copymerge.getUnderlyingCounts()->Clone();
  TH2::AddDirectory(bStatus);

  if (!mUnderlyingCounts) {
    return;
  }
  mUnderlyingCounts->Sumw2();
  update();
}

void TH2Fraction::update()
{
  if (mEventCounter) {
    for (int i = 1; i <= GetXaxis()->GetNbins(); i++) {
      for (int j = 1; j <= GetYaxis()->GetNbins(); j++) {
        SetBinContent(i, j, mUnderlyingCounts->GetBinContent(i, j) / mEventCounter);
        SetBinError(i, j, mUnderlyingCounts->GetBinError(i, j) / mEventCounter);
      }
    }
  }
}

void TH2Fraction::merge(MergeInterface* const other)
{
  // special merge method to approximately combine two histograms
  auto otherHisto = dynamic_cast<const TH2Fraction* const>(other);
  if (otherHisto) {
    if (mUnderlyingCounts->Add(otherHisto->getUnderlyingCounts())) {
      mEventCounter += otherHisto->getEventCounter();
    }
  }
  update();
}

void TH2Fraction::Reset(Option_t* option)
{
  if (mUnderlyingCounts) {
    mUnderlyingCounts->Reset(option);
  }
  mEventCounter = 0;
  TH2D::Reset(option);
}

} // namespace o2::quality_control_modules::phos
