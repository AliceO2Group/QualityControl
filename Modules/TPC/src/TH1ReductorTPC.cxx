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
#include <TAxis.h>
#include <TCanvas.h>
#include <TList.h>
#include <fmt/format.h>

namespace o2::quality_control_modules::tpc
{
void TH1ReductorTPC::update(TObject* obj, std::vector<SliceInfo>& reducedSource,
                            std::vector<std::vector<float>>& axis,
                            std::vector<std::string>& ranges)
{
  // Define the local variables in the default case: 1 single pad
  // (no multipad canvas, nor slicer), and slicer axes size set to 1 (no slicing).
  TList* padList = nullptr; // List of TPads if input TCanvas.
  TH1* histo = nullptr;     // Pointer to the histogram to trend.
  int numberPads = 1;       // Number of input objects.
  int numberSlices = 1;     // Default value for the inner slicer axis.
  bool useSlicing = false;

  // GANESHA add protection that axisSize == 1. But, do we allow that the json contains also y or z axis i.e. or protection would be if( (int)axis.size() < 1 )
  if ((int)axis.size() != 1) {
    ILOG(Error, Support) << "Error: 'axisDivision' in json not configured properly for TH1Reductor. Should contain exactly one axis." << ENDM;
  }

  // Get the number of pads, and their list in case of an input canvas.
  const bool isCanvas = (obj->IsA() == TCanvas::Class());
  if (isCanvas) {
    auto canvas = static_cast<TCanvas*>(obj);
    padList = static_cast<TList*>(canvas->GetListOfPrimitives());
    padList->SetOwner(kTRUE);
    numberPads = padList->GetEntries();
  } else {                                    // Non-canvas case: number of input pads always 1, i.e. only one histogram passed.
    if (axis[0].size() > 1) {                 // Ensure the slicer config is valid, either to select a certain range, or obtain multiple slices.
                                              // Otherwise, full range of histo will be used.
      numberSlices = (int)axis[0].size() - 1; // axis[0].size() = number of boundaries.
      useSlicing = true;                      // Enable the use of custom boundaries.
    } else {
      ILOG(Info, Support) << "Not enough axis boundaries for slicing. Will use full histogram range." << ENDM;
    }
  }
  ILOG(Info, Support) << "Number of input histograms for the trending of "
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
      // Bin Numbers for correctly getting the statistical properties
      int BinXLow = 0;
      int BinXUp = 0;

      // Get the trending quantities defined in 'SlicerInfo'.
      for (int j = 0; j < numberSlices; j++) {
        std::string thisRange;
        float SliceLabel = 0.;

        if (useSlicing) {
          BinXLow = histo->GetXaxis()->FindBin(axis[0][j]);
          if (axis[0][j] > histo->GetXaxis()->GetBinCenter(BinXLow)) {
            BinXLow += 1;
          } // Lower slice boundary is above bin center. Start at next higher bin
          BinXUp = histo->GetXaxis()->FindBin(axis[0][j + 1]);
          if (axis[0][j + 1] <= histo->GetXaxis()->GetBinCenter(BinXUp)) {
            BinXUp -= 1;
          } // Upper slice boundary is smaller equal bin center. Stop at next lower bin

          histo->GetXaxis()->SetRange(BinXLow, BinXUp);
          thisRange = fmt::format("{0:s} - RangeX: [{1:.1f}, {2:.1f}]", histo->GetTitle(), axis[0][j], axis[0][j + 1]);
          SliceLabel = (axis[0][j] + axis[0][j + 1]) / 2.;
        } else {
          if (isCanvas) {
            thisRange = fmt::format("{0:s}", histo->GetTitle());
            SliceLabel = (float)(j);
          } else {
            thisRange = fmt::format("{0:s} - RangeX (default): [{1:.1f}, {2:.1f}]", histo->GetTitle(), histo->GetXaxis()->GetXmin(), histo->GetXaxis()->GetXmax());
            SliceLabel = (histo->GetXaxis()->GetXmin() + histo->GetXaxis()->GetXmax()) / 2.;
          }
          BinXLow = 1;
          BinXUp = histo->GetNbinsX();
        }
        ranges.push_back(thisRange);

        SliceInfo mySlice;
        mySlice.entries = histo->Integral(BinXLow, BinXUp);
        mySlice.meanX = histo->GetMean(1);
        mySlice.stddevX = histo->GetStdDev(1);
        if (mySlice.entries != 0) {
          mySlice.errMeanX = mySlice.stddevX / (sqrt(mySlice.entries));
        } else {
          mySlice.errMeanX = 0.;
        }

        float StatsY[3]; // 0 Mean, 1 Stddev, 2 Error
        if (useSlicing) {
          GetTH1StatsY(histo, StatsY, BinXLow, BinXUp);
        } else { // We don't slice and take the full histo as defined.
          GetTH1StatsY(histo, StatsY, BinXLow, BinXUp);
        }

        mySlice.meanY = StatsY[0];
        mySlice.stddevY = StatsY[1];
        mySlice.errMeanY = StatsY[2];
        mySlice.sliceLabelX = SliceLabel;
        mySlice.sliceLabelY = 0.;

        reducedSource.emplace_back(mySlice);
      }

    } else {
      ILOG(Error, Support) << "Error: 'histo' not found." << ENDM;
    }
  } // All the vector elements have been updated.
}

void TH1ReductorTPC::GetTH1StatsY(TH1* Hist, float Stats[3],
                                  const int LowerBin, const int UpperBin)
{
  const int NTotalBins = Hist->GetNbinsX();
  const int IterateBins = UpperBin - LowerBin + 1; // Amount of bins included in the calculation.
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

  float MeanY = 0.;
  float StddevY = 0.;
  float ErrMeanY = 0.;

  for (int i = LowerBin; i <= UpperBin; i++) {
    MeanY += Hist->GetBinContent(i);
  }

  MeanY /= (float)IterateBins;

  for (int i = LowerBin; i <= UpperBin; i++) {
    StddevY += pow(MeanY - Hist->GetBinContent(i), 2.);
  }

  StddevY /= ((float)IterateBins - 1.);
  ErrMeanY = StddevY / ((float)IterateBins);
  StddevY = sqrt(StddevY);
  ErrMeanY = sqrt(ErrMeanY);

  Stats[0] = MeanY;
  Stats[1] = StddevY;
  Stats[2] = ErrMeanY;
} // TH1ReductorTPC::GetTH1StatsY(TH1* Hist, float Stats[3], float LowerBoundary, float UpperBoundary)

} // namespace o2::quality_control_modules::tpc
