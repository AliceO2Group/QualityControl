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
/// \file     TH1Reductor.cxx
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#include "QualityControl/QcInfoLogger.h"
#include "TPC/TH1ReductorTPC.h"
#include <vector>
#include <TAxis.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TList.h>

namespace o2::quality_control_modules::tpc
{
void TH1ReductorTPC::update(TObject* obj, std::vector<SliceInfo>& reducedSource,
                            std::vector<std::vector<float>>& axis)
{
  // Define the local variables in the default case: 1 single pad
  // (no multipad canvas, nor slicer), and slicer axes size set to 1 (no slicing).
  TList* padList = nullptr; // List of TPads if input TCanvas.
  TH1* histo = nullptr;     // Pointer to the histogram to trend.
  int numberPads = 1;       // Number of output objects.
  int axisSize = 1;         // Default value for the main slicer axis.
  int innerAxisSize = 1;    // Default value for the inner slicer axis.

  // Get the number of pads, and their list in case of an input canvas.
  const bool isCanvas = (obj->IsA() == TCanvas::Class());
  if (isCanvas) {
    auto canvas = static_cast<TCanvas*>(obj);
    padList = static_cast<TList*>(canvas->GetListOfPrimitives());
    padList->SetOwner(kTRUE);
    numberPads = padList->GetEntries();
  } else {                    // Slicer case: number of pads = axis.size*(innerAxis.size-1).
    if (axis[0].size() > 1) { // Ensure the slicer config is valid.
      axisSize = (int)axis.size();
      innerAxisSize = (int)axis[0].size() - 1;
    }
  }
  ILOG(Info, Support) << "Number of output histograms for the trending of "
                      << obj->GetName() << ": " << numberPads << ENDM;

  // Access the histograms embedded in 'obj'.
  for (int iPad = 0; iPad < numberPads; iPad++) {
    if (isCanvas) {
      auto pad = static_cast<TPad*>(padList->At(iPad));
      histo = static_cast<TH1*>(pad->GetListOfPrimitives()->At(0));
    } else { // 'obj' is already a single histogram.
      histo = static_cast<TH1*>(obj);
    }

    if (histo) {
      // Get the trending quantities defined in 'SlicerInfo'.
      // The two for-loop do only one pass if we have an input canvas.
      for (int i = 0; i < axisSize; i++) {
        for (int j = 0; j < innerAxisSize; j++) {
          if (!isCanvas && (innerAxisSize > 1)) {
            histo->GetXaxis()->SetRangeUser(axis[i][j], axis[i][j + 1]);
          }

          SliceInfo mySlice;
          mySlice.entries = histo->GetEntries();
          mySlice.meanX = histo->GetMean(1);
          mySlice.stddevX = histo->GetStdDev(1);
          mySlice.errMeanX = mySlice.stddevX / (sqrt(mySlice.entries));

          float StatsY[3]; // 0 Mean, 1 Stddev, 2 Error
          GetTH1StatsY(histo, StatsY, axis[i][j], axis[i][j + 1]);

          mySlice.meanY = StatsY[0];
          mySlice.stddevY = StatsY[1];
          mySlice.errMeanY = StatsY[2];

          reducedSource.emplace_back(mySlice);
        }
      }

    } else {
      ILOG(Error, Support) << "Error: 'histo' not found." << ENDM;
    }
  } // All the vector elements have been updated.
}

void TH1ReductorTPC::GetTH1StatsY(TH1* Hist, float Stats[3],
                                  float LowerBoundary, float UpperBoundary)
{
  const int LowerBin = Hist->FindBin(LowerBoundary);
  const int UpperBin = Hist->FindBin(UpperBoundary);
  const int NTotalBins = Hist->GetNbinsX();
  const float IterateBins = (float)UpperBin - (float)LowerBin + 1.; // Amount of bins included in the calculation.
                                                                    // Includes LowerBin and UpperBin.

  // Safety measures.
  if (LowerBin <= 0 || UpperBin <= 0) {
    ILOG(Error, Support) << "Error: Negative bin in TH1ReducterTPC::GetTH1StatsY" << ENDM;
    exit(0);
  }
  if (UpperBin <= LowerBin) {
    ILOG(Error, Support) << "Error: Upper bin smaller than lower bin in TH1ReducterTPC::GetTH1StatsY" << ENDM;
    exit(0);
  }
  if (NTotalBins < (UpperBin - LowerBin)) {
    ILOG(Error, Support) << "Error: Bin region bigger than total amount of bins TH1ReducterTPC::GetTH1StatsY" << ENDM;
    exit(0);
  }

  float SumY = 0.;
  float SumY2 = 0.;

  for (int i = LowerBin; i <= UpperBin; i++) {
    SumY += Hist->GetBinContent(i);
    SumY2 += Hist->GetBinContent(i) * Hist->GetBinContent(i);
  }

  float MeanY = SumY / IterateBins;
  float StddevY = (SumY2 - SumY * SumY / IterateBins) / (IterateBins - 1.);
  float ErrMeanY = StddevY / IterateBins;

  Stats[0] = MeanY;
  Stats[1] = sqrt(StddevY);
  Stats[2] = sqrt(ErrMeanY);
} // TH1ReductorTPC::GetTH1StatsY(TH1* Hist, float Stats[3], float LowerBoundary, float UpperBoundary)

} // namespace o2::quality_control_modules::tpc
