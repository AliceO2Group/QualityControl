/// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
/// See https://alice-o2.web.cern.ch/copyright for details of the copyright
/// holders. All rights not expressly granted are reserved.
//
/// This software is distributed under the terms of the GNU General Public
/// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
/// In applying this license CERN does not waive the privileges and immunities
/// granted to it by virtue of its status as an Intergovernmental Organization
/// or submit itself to any jurisdiction.

///
/// \file   TH1ReductorLaser.cxx
/// \author Piotr Konopka, developed to laser QC by Sandor Lokos
/// (sandor.lokos@cern.ch)
///

#include "FT0/TH1ReductorLaser.h"
#include <TF1.h>
#include <TH1.h>
namespace o2::quality_control_modules::ft0
{
void* TH1ReductorLaser::getBranchAddress() { return &mStats; }
const char* TH1ReductorLaser::getBranchLeafList()
{
  return "mean/D:mean1fit/D:mean2fit/D:stddev:stddev1fit:stddev2fit:entries";
}
double Gauss(double* x, double* par)
{
  double arg = 0;
  const int PI = 3.1415926536;
  if (par[2] != 0) {
    arg = (x[0] - par[1]) / par[2] ;
  }
  double fitval = par[0] / (par[2] * sqrt( 2 * PI )) * exp(-0.5 * arg * arg);
  return fitval;
}

void TH1ReductorLaser::update(TObject* obj)
{
  auto histo = dynamic_cast<TH1*>(obj);
  if (histo) {
    /// Store the mean and the stddev (makes sense if there is one peak)
    if (histo->GetBinCenter(1) != 0)
      histo->GetXaxis()->SetRangeUser(1, histo->GetEntries() - 1);

    double origMean = histo->GetMean();
    double origStddev = histo->GetStdDev();
    mStats.entries = histo->GetEntries();
    /// Set parameters for single peak
    mStats.mean = origMean;
    mStats.stddev = origStddev;

    /// Store the original xmin and xmax
    double origLowerLimit = histo->GetBinCenter(1);
    double origUpperLimit = histo->GetBinCenter(histo->GetEntries() - 1);

    /// Get the first peak and have a guess of its Gaussian parameters
    /// The parameters from the fit cannot be too far from the initial guess
    /// After that the original limits of the histo must be restored
    histo->GetXaxis()->SetRangeUser(histo->GetMean() - 1.25 * histo->GetStdDev(), histo->GetMean() - 0.25 * histo->GetStdDev());
    double peak1Amp = histo->GetBin(histo->GetMean());
    double peak1Pos = histo->GetMean();
    double peak1Wid = histo->GetStdDev();

    TF1* GaussFit1 = new TF1("gauss1", Gauss, histo->GetStdDev(), histo->GetStdDev(), 3);

    GaussFit1->SetParameter(0, peak1Amp);
    GaussFit1->SetParameter(1, peak1Pos);
    GaussFit1->SetParameter(2, peak1Wid);
    histo->Fit("gauss1");

    mStats.mean1fit = GaussFit1->GetParameter(1);
    mStats.stddev1fit = GaussFit1->GetParameter(2);

    /// Restoring the original limits
    histo->GetXaxis()->SetRangeUser(origLowerLimit, origUpperLimit);

    /// Get the second peak and have a guess of its Gaussian parameters
    /// The parameters from the fit cannot be too far from the initial guess
    /// After that the original limits of the histo must be restored
    histo->GetXaxis()->SetRangeUser(histo->GetMean() + 0.25 * histo->GetStdDev(), histo->GetMean() + 1.25 * histo->GetStdDev());
    double peak2Amp = histo->GetBin(histo->GetMean());
    double peak2Pos = histo->GetMean();
    double peak2Wid = histo->GetStdDev();

    TF1* GaussFit2 = new TF1("gauss2", Gauss, histo->GetStdDev(), histo->GetStdDev(), 3);

    GaussFit2->SetParameter(0, peak2Amp);
    GaussFit2->SetParameter(1, peak2Pos);
    GaussFit2->SetParameter(2, peak2Wid);
    histo->Fit("gauss2");

    mStats.mean2fit = GaussFit2->GetParameter(1);
    mStats.stddev2fit = GaussFit2->GetParameter(2);
  }
}
} // namespace o2::quality_control_modules::ft0
