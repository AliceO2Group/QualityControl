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

/// \file MergeableTH2Ratio.cxx
/// \brief An example of a custom TH2Quotient inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Sebastien Perrin, Andrea Ferrero

#include "MUONCommon/MergeableTH2Ratio.h"

using namespace std;
namespace o2::quality_control_modules::muon
{

MergeableTH2Ratio::MergeableTH2Ratio(MergeableTH2Ratio const& copymerge)
  : TH2F(copymerge.GetName(), copymerge.GetTitle(),
         copymerge.getNum()->GetXaxis()->GetNbins(), copymerge.getNum()->GetXaxis()->GetXmin(), copymerge.getNum()->GetXaxis()->GetXmax(),
         copymerge.getNum()->GetYaxis()->GetNbins(), copymerge.getNum()->GetYaxis()->GetXmin(), copymerge.getNum()->GetYaxis()->GetXmax()),
    o2::mergers::MergeInterface(),
    // mlistOfFunctions(copymerge.getNum()->GetListOfFunctions()),
    mlistOfFunctions(nullptr),
    mScalingFactor(copymerge.getScalingFactor())
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = (TH2F*)copymerge.getNum()->Clone();
  mHistoDen = (TH2F*)copymerge.getDen()->Clone();
  TH1::AddDirectory(bStatus);
}

MergeableTH2Ratio::MergeableTH2Ratio(const char* name, const char* title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double ymax, double scaling)
  : TH2F(name, title, nbinsx, xmin, xmax, nbinsy, ymin, ymax),
    o2::mergers::MergeInterface(),
    // mlistOfFunctions(mHistoNum->GetListOfFunctions()),
    mlistOfFunctions(nullptr),
    mScalingFactor(scaling)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new TH2F("num", "num", nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  mHistoDen = new TH2F("den", "den", nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  TH1::AddDirectory(bStatus);
  update();
}

MergeableTH2Ratio::MergeableTH2Ratio(const char* name, const char* title, double scaling)
  : TH2F(name, title, 10, 0, 10, 10, 0, 10),
    o2::mergers::MergeInterface(),
    // mlistOfFunctions(mHistoNum->GetListOfFunctions()),
    mlistOfFunctions(nullptr),
    mScalingFactor(scaling)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new TH2F("num", "num", 10, 0, 10, 10, 0, 10);
  mHistoDen = new TH2F("den", "den", 10, 0, 10, 10, 0, 10);
  TH1::AddDirectory(bStatus);
  update();
}

MergeableTH2Ratio::~MergeableTH2Ratio()
{
  if (mHistoNum) {
    delete mHistoNum;
  }

  if (mHistoDen) {
    delete mHistoDen;
  }
}

void MergeableTH2Ratio::merge(MergeInterface* const other)
{
  mHistoNum->Add(dynamic_cast<const MergeableTH2Ratio* const>(other)->getNum());
  mHistoDen->Add(dynamic_cast<const MergeableTH2Ratio* const>(other)->getDen());
  update();
}

void MergeableTH2Ratio::update()
{
  static constexpr double sOrbitLengthInNanoseconds = 3564 * 25;
  static constexpr double sOrbitLengthInMicroseconds = sOrbitLengthInNanoseconds / 1000;
  static constexpr double sOrbitLengthInMilliseconds = sOrbitLengthInMicroseconds / 1000;
  const char* name = this->GetName();
  const char* title = this->GetTitle();
  Reset();
  // if(mlistOfFunctions->GetLast() > 0){
  //     beautify();
  // }
  GetXaxis()->Set(mHistoNum->GetXaxis()->GetNbins(), mHistoNum->GetXaxis()->GetXmin(), mHistoNum->GetXaxis()->GetXmax());
  GetYaxis()->Set(mHistoNum->GetYaxis()->GetNbins(), mHistoNum->GetYaxis()->GetXmin(), mHistoNum->GetYaxis()->GetXmax());
  SetBinsLength();
  Divide(mHistoNum, mHistoDen);
  SetNameTitle(name, title);
  // convertion to KHz units
  if (mScalingFactor != 1.) {
    Scale(1. / sOrbitLengthInMilliseconds);
  }
  SetOption("colz");
}

void MergeableTH2Ratio::beautify()
{
  // GetListOfFunctions()->RemoveAll();
  // GetListOfFunctions()->AddAll(mlistOfFunctions);
}

} // namespace o2::quality_control_modules::muon
