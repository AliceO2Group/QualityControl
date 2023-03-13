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
#include <TList.h>

#include <fmt/format.h>
#include <limits>

using namespace std;
namespace o2::quality_control_modules::muon
{

MergeableTH2Ratio::MergeableTH2Ratio(MergeableTH2Ratio const& copymerge)
  : TH2F(copymerge.GetName(), copymerge.GetTitle(),
         copymerge.getNum()->GetXaxis()->GetNbins(), copymerge.getNum()->GetXaxis()->GetXmin(), copymerge.getNum()->GetXaxis()->GetXmax(),
         copymerge.getNum()->GetYaxis()->GetNbins(), copymerge.getNum()->GetYaxis()->GetXmin(), copymerge.getNum()->GetYaxis()->GetXmax()),
    o2::mergers::MergeInterface(),
    mShowZeroBins(copymerge.getShowZeroBins())
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = (TH2F*)copymerge.getNum()->Clone();
  mHistoDen = (TH2F*)copymerge.getDen()->Clone();
  TH1::AddDirectory(bStatus);
}

MergeableTH2Ratio::MergeableTH2Ratio(const char* name, const char* title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double ymax, bool showZeroBins)
  : TH2F(name, title, nbinsx, xmin, xmax, nbinsy, ymin, ymax),
    o2::mergers::MergeInterface(),
    mShowZeroBins(showZeroBins)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new TH2F("num", "num", nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  mHistoDen = new TH2F("den", "den", nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  TH1::AddDirectory(bStatus);
  update();
}

MergeableTH2Ratio::MergeableTH2Ratio(const char* name, const char* title, bool showZeroBins)
  : TH2F(name, title, 10, 0, 10, 10, 0, 10),
    o2::mergers::MergeInterface(),
    mShowZeroBins(showZeroBins)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new TH2F("num", "num", 10, 0, 10, 10, 0, 10);
  mHistoDen = new TH2F("den", "den", 10, 0, 10, 10, 0, 10);
  TH1::AddDirectory(bStatus);
  update();
}

MergeableTH2Ratio::MergeableTH2Ratio() : TH2F(), o2::mergers::MergeInterface(), mShowZeroBins(false)
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
  if (!mHistoNum || !mHistoDen) {
    return;
  }

  mHistoNum->Add(dynamic_cast<const MergeableTH2Ratio* const>(other)->getNum());
  mHistoDen->Add(dynamic_cast<const MergeableTH2Ratio* const>(other)->getDen());
  update();
}

void MergeableTH2Ratio::update()
{
  if (!mHistoNum || !mHistoDen) {
    return;
  }

  const char* name = this->GetName();
  const char* title = this->GetTitle();

  TH2F::Reset();

  SetNameTitle(name, title);
  GetXaxis()->Set(mHistoNum->GetXaxis()->GetNbins(), mHistoNum->GetXaxis()->GetXmin(), mHistoNum->GetXaxis()->GetXmax());
  GetYaxis()->Set(mHistoNum->GetYaxis()->GetNbins(), mHistoNum->GetYaxis()->GetXmin(), mHistoNum->GetYaxis()->GetXmax());
  SetBinsLength();
  beautify();

  Divide(mHistoNum, mHistoDen);

  if (mShowZeroBins) {
    // bins which have zero numerators are plotted in white when using the "col" and "colz" options, regardless of
    // the contents of the denominators. However, it is sometimes useful to distinguish between bins with zero
    // denominators (no information), and those with non-zero denominators and zero numerators.
    // In the latter case we set the bin content in the ratio to a value slightly higher than zero, such that they
    // will be plotted in dark-blue and can be distinguished from those with no information, which will still be
    // plotted in white
    for (int binx = 1; binx <= mHistoNum->GetXaxis()->GetNbins(); binx++) {
      for (int biny = 1; biny <= mHistoNum->GetYaxis()->GetNbins(); biny++) {
        if (mHistoNum->GetBinContent(binx, biny) == 0 && mHistoDen->GetBinContent(binx, biny) != 0) {
          SetBinContent(binx, biny, std::numeric_limits<float>::min());
          SetBinError(binx, biny, 1);
        }
      }
    }
  }
}

void MergeableTH2Ratio::beautify()
{
  if (!mHistoNum) {
    return;
  }

  GetListOfFunctions()->RemoveAll();

  TList* functions = (TList*)mHistoNum->GetListOfFunctions()->Clone();
  if (functions) {
    GetListOfFunctions()->AddAll(functions);
    delete functions;
  }
}

void MergeableTH2Ratio::Reset(Option_t* option)
{
  if (mHistoNum) {
    mHistoNum->Reset(option);
  }

  if (mHistoDen) {
    mHistoDen->Reset(option);
  }

  TH2F::Reset(option);
}

void MergeableTH2Ratio::Copy(TObject& obj) const
{
  auto dest = dynamic_cast<MergeableTH2Ratio*>(&obj);
  if (!dest) {
    return;
  }

  getNum()->Copy(*(dest->getNum()));
  getDen()->Copy(*(dest->getDen()));
  dest->setShowZeroBins(getShowZeroBins());
  TH2F::Copy(obj);

  dest->update();
}

Bool_t MergeableTH2Ratio::Add(const TH1* h1, const TH1* h2, Double_t c1, Double_t c2)
{
  auto m1 = dynamic_cast<const MergeableTH2Ratio*>(h1);
  if (!m1) {
    return kFALSE;
  }

  auto m2 = dynamic_cast<const MergeableTH2Ratio*>(h2);
  if (!m2) {
    return kFALSE;
  }

  if (!getNum()->Add(m1->getNum(), m2->getNum(), c1, c2)) {
    return kFALSE;
  }
  if (!getDen()->Add(m1->getDen(), m2->getDen(), c1, c2)) {
    return kFALSE;
  }

  update();
  return kTRUE;
}

Bool_t MergeableTH2Ratio::Add(const TH1* h1, Double_t c1)
{
  auto m1 = dynamic_cast<const MergeableTH2Ratio*>(h1);
  if (!m1) {
    return kFALSE;
  }

  if (!getNum()->Add(m1->getNum(), c1)) {
    return kFALSE;
  }
  if (!getDen()->Add(m1->getDen(), c1)) {
    return kFALSE;
  }

  update();
  return kTRUE;
}

void MergeableTH2Ratio::SetBins(Int_t nx, Double_t xmin, Double_t xmax)
{
  std::cout << "SetBins(Int_t nx, Double_t xmin, Double_t xmax) not valid for MergeableTH2Ratio" << std::endl;
}

void MergeableTH2Ratio::SetBins(Int_t nx, Double_t xmin, Double_t xmax,
                                Int_t ny, Double_t ymin, Double_t ymax)
{
  getNum()->SetBins(nx, xmin, xmax, ny, ymin, ymax);
  getDen()->SetBins(nx, xmin, xmax, ny, ymin, ymax);
  TH2F::SetBins(nx, xmin, xmax, ny, ymin, ymax);
}

void MergeableTH2Ratio::SetBins(Int_t nx, Double_t xmin, Double_t xmax,
                                Int_t ny, Double_t ymin, Double_t ymax,
                                Int_t nz, Double_t zmin, Double_t zmax)
{
  std::cout << "SetBins(Int_t nx, Double_t xmin, Double_t xmax, Int_t ny, Double_t ymin, Double_t ymax, Int_t nz, Double_t zmin, Double_t zmax) not valid for MergeableTH2Ratio" << std::endl;
}

} // namespace o2::quality_control_modules::muon
