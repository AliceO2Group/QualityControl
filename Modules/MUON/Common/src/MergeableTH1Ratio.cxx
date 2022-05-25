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

/// \file MergeableTH1Ratio.cxx
/// \brief An example of a custom TH2Quotient inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Sebastien Perrin, Andrea Ferrero

#include "MUONCommon/MergeableTH1Ratio.h"

using namespace std;
namespace o2::quality_control_modules::muon
{

MergeableTH1Ratio::MergeableTH1Ratio(MergeableTH1Ratio const& copymerge)
  : TH1F(copymerge.GetName(), copymerge.GetTitle(),
         copymerge.getNum()->GetXaxis()->GetNbins(), copymerge.getNum()->GetXaxis()->GetXmin(), copymerge.getNum()->GetXaxis()->GetXmax()),
    o2::mergers::MergeInterface(),
    mScalingFactor(copymerge.getScalingFactor())
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = (TH1D*)copymerge.getNum()->Clone();
  mHistoDen = (TH1D*)copymerge.getDen()->Clone();
  TH1::AddDirectory(bStatus);

  mHistoNum->Sumw2();
  mHistoDen->Sumw2();
}

MergeableTH1Ratio::MergeableTH1Ratio(const char* name, const char* title, int nbinsx, double xmin, double xmax, double scaling)
  : TH1F(name, title, nbinsx, xmin, xmax),
    o2::mergers::MergeInterface(),
    mScalingFactor(scaling)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new TH1D("num", "num", nbinsx, xmin, xmax);
  mHistoDen = new TH1D("den", "den", nbinsx, xmin, xmax);
  TH1::AddDirectory(bStatus);

  mHistoNum->Sumw2();
  mHistoDen->Sumw2();

  update();
}

MergeableTH1Ratio::MergeableTH1Ratio(const char* name, const char* title, double scaling)
  : TH1F(name, title, 10, 0, 10),
    o2::mergers::MergeInterface(),
    mScalingFactor(scaling)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new TH1D("num", "num", 10, 0, 10);
  mHistoDen = new TH1D("den", "den", 10, 0, 10);
  TH1::AddDirectory(bStatus);

  mHistoNum->Sumw2();
  mHistoDen->Sumw2();

  update();
}

MergeableTH1Ratio::~MergeableTH1Ratio()
{
  if (mHistoNum) {
    delete mHistoNum;
  }

  if (mHistoDen) {
    delete mHistoDen;
  }
}

void MergeableTH1Ratio::merge(MergeInterface* const other)
{
  mHistoNum->Add(dynamic_cast<const MergeableTH1Ratio* const>(other)->getNum());
  mHistoDen->Add(dynamic_cast<const MergeableTH1Ratio* const>(other)->getDen());
  update();
}

void MergeableTH1Ratio::update()
{
  const char* name = this->GetName();
  const char* title = this->GetTitle();

  Reset();
  GetXaxis()->Set(mHistoNum->GetXaxis()->GetNbins(), mHistoNum->GetXaxis()->GetXmin(), mHistoNum->GetXaxis()->GetXmax());
  SetBinsLength();

  // Compute the ratio with a temporary TH1D, then copy the bin contents and errors
  TH1D* hRatio = (TH1D*)mHistoNum->Clone();
  hRatio->Divide(mHistoDen);

  for (int i = 1; i <= GetXaxis()->GetNbins(); i++) {
    SetBinContent(i, hRatio->GetBinContent(i));
    SetBinError(i, hRatio->GetBinError(i));
  }
  SetNameTitle(name, title);

  delete hRatio;

  // scale plot if needed
  if (mScalingFactor != 1.) {
    Scale(mScalingFactor);
  }
}

} // namespace o2::quality_control_modules::muon
