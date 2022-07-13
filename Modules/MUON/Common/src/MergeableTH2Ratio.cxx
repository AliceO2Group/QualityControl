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
  const char* name = this->GetName();
  const char* title = this->GetTitle();

  Reset();
  beautify();

  GetXaxis()->Set(mHistoNum->GetXaxis()->GetNbins(), mHistoNum->GetXaxis()->GetXmin(), mHistoNum->GetXaxis()->GetXmax());
  GetYaxis()->Set(mHistoNum->GetYaxis()->GetNbins(), mHistoNum->GetYaxis()->GetXmin(), mHistoNum->GetYaxis()->GetXmax());
  SetBinsLength();

  Divide(mHistoNum, mHistoDen);
  SetNameTitle(name, title);

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
          SetBinContent(binx, biny, 0.000001);
          SetBinError(binx, biny, 1);
        }
      }
    }
  }
}

void MergeableTH2Ratio::beautify()
{
  SetOption("colz");
  GetListOfFunctions()->RemoveAll();

  TList* functions = (TList*)mHistoNum->GetListOfFunctions()->Clone();
  if (functions) {
    GetListOfFunctions()->AddAll(functions);
    delete functions;
  }
}

} // namespace o2::quality_control_modules::muon
