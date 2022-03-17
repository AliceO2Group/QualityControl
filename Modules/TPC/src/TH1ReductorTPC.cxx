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
      int binXLow = 0;
      int binXUp = 0;

      // Get the trending quantities defined in 'SlicerInfo'.
      for (int j = 0; j < numberSlices; j++) {
        std::string thisRange;
        float sliceLabel = 0.;

        if (useSlicing) {
          getBinSlices(histo->GetXaxis(), axis[0][j], axis[0][j + 1], binXLow, binXUp, sliceLabel);
          histo->GetXaxis()->SetRange(binXLow, binXUp);
          thisRange = fmt::format("{0:s} - RangeX: [{1:.1f}, {2:.1f}]", histo->GetTitle(), axis[0][j], axis[0][j + 1]);
        } else {
          if (isCanvas) {
            thisRange = fmt::format("{0:s}", histo->GetTitle());
            sliceLabel = (float)(j);
          } else {
            thisRange = fmt::format("{0:s} - RangeX (default): [{1:.1f}, {2:.1f}]", histo->GetTitle(), histo->GetXaxis()->GetXmin(), histo->GetXaxis()->GetXmax());
            sliceLabel = (histo->GetXaxis()->GetXmin() + histo->GetXaxis()->GetXmax()) / 2.;
          }
          binXLow = 1;
          binXUp = histo->GetNbinsX();
        }
        ranges.push_back(thisRange);

        SliceInfo mySlice;
        mySlice.entries = histo->Integral(binXLow, binXUp);
        mySlice.meanX = histo->GetMean(1);
        mySlice.stddevX = histo->GetStdDev(1);
        if (mySlice.entries != 0) {
          mySlice.errMeanX = mySlice.stddevX / (sqrt(mySlice.entries));
        } else {
          mySlice.errMeanX = 0.;
        }

        float StatsY[3]; // 0 Mean, 1 Stddev, 2 Error
        if (useSlicing) {
          GetTH1StatsY(histo, StatsY, binXLow, binXUp);
        } else { // We don't slice and take the full histo as defined.
          GetTH1StatsY(histo, StatsY, binXLow, binXUp);
        }

        mySlice.meanY = StatsY[0];
        mySlice.stddevY = StatsY[1];
        mySlice.errMeanY = StatsY[2];
        mySlice.sliceLabelX = sliceLabel;
        mySlice.sliceLabelY = 0.;

        reducedSource.emplace_back(mySlice);
      }

    } else {
      ILOG(Error, Support) << "Error: 'histo' not found." << ENDM;
    }
  } // All the vector elements have been updated.
}

void TH1ReductorTPC::GetTH1StatsY(TH1* hist, float stats[3],
                                  const int lowerBin, const int upperBin)
{
  const int nTotalBins = hist->GetNbinsX();
  const int iterateBins = upperBin - lowerBin + 1; // Amount of bins included in the calculation.
                                                   // Includes lowerBin and upperBin.

  // Safety measures.
  if (lowerBin <= 0 || upperBin <= 0) {
    ILOG(Error, Support) << "Error: Negative bin in TH1ReducterTPC::GetTH1StatsY" << ENDM;
    exit(0);
  }
  if (upperBin <= lowerBin) {
    ILOG(Error, Support) << "Error: Upper bin smaller than lower bin in TH1ReducterTPC::GetTH1StatsY" << ENDM;
    exit(0);
  }
  if (nTotalBins < (upperBin - lowerBin)) {
    ILOG(Error, Support) << "Error: Bin region bigger than total amount of bins TH1ReducterTPC::GetTH1StatsY" << ENDM;
    exit(0);
  }

  float meanY = 0.;
  float stddevY = 0.;
  float errMeanY = 0.;

  for (int i = lowerBin; i <= upperBin; i++) {
    meanY += hist->GetBinContent(i);
  }

  meanY /= (float)iterateBins;

  for (int i = lowerBin; i <= upperBin; i++) {
    stddevY += pow(meanY - hist->GetBinContent(i), 2.);
  }

  stddevY /= ((float)iterateBins - 1.);
  errMeanY = stddevY / ((float)iterateBins);
  stddevY = sqrt(stddevY);
  errMeanY = sqrt(errMeanY);

  stats[0] = meanY;
  stats[1] = stddevY;
  stats[2] = errMeanY;
} // TH1ReductorTPC::GetTH1StatsY(TH1* hist, float stats[3], float LowerBoundary, float UpperBoundary)

} // namespace o2::quality_control_modules::tpc
