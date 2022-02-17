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
/// \file     TH2Reductor.cxx
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#include "QualityControl/QcInfoLogger.h"
#include "TPC/TH2ReductorTPC.h"
#include <TAxis.h>
#include <TCanvas.h>
#include <TH2.h>
#include <TList.h>

namespace o2::quality_control_modules::tpc
{
void TH2ReductorTPC::update(TObject* obj, std::vector<SliceInfo>& reducedSource,
                            std::vector<std::vector<float>>& axis,
                            std::vector<std::string>& ranges)
{
  // Define the local variables in the default case: 1 single pad
  // (no multipad canvas, nor slicer), and slicer axes size set to 1 (no slicing).
  TList* padList = nullptr; // List of TPads if input TCanvas.
  TH2* histo = nullptr;     // Pointer to the histogram to trend.
  int numberPads = 1;       // Number of input objects.
  int numberSlicesX = 1;    // Default value for the X axis slicer.
  int numberSlicesY = 1;    // Default value for the Y axis slicer.
  bool useSlicingX = false;
  bool useSlicingY = false;

  // GANESHA add protection that number of axes == 2.
  if ((int)axis.size() != 2) {
    ILOG(Error, Support) << "Error: 'axisDivision' in json not configured properly for TH2Reductor. Should contain exactly two axes." << ENDM;
  }

  // Get the number of pads, and their list in case of an input canvas.
  const bool isCanvas = (obj->IsA() == TCanvas::Class());
  if (isCanvas) {
    auto canvas = static_cast<TCanvas*>(obj);
    padList = static_cast<TList*>(canvas->GetListOfPrimitives());
    padList->SetOwner(kTRUE);
    numberPads = padList->GetEntries();
  } else {                                     // Non-canvas case: number of input pads always 1, i.e. only one histogram passed.
    if (axis[0].size() > 1) {                  // Ensure the slicer config X is valid, either to select a certain range, or obtain multiple slices.
                                               // Otherwise, full range of histo will be used.
      numberSlicesX = (int)axis[0].size() - 1; // axis[0].size() = number of boundaries.
      useSlicingX = true;                      // Enable the use of custom boundaries.
    } else {
      ILOG(Info, Support) << "Not enough axis boundaries for slicing on X. Will use full histogram range along X." << ENDM;
    }

    if (axis[1].size() > 1) {                  // Ensure the slicer config Y is valid, either to select a certain range, or obtain multiple slices.
                                               // Otherwise, full range of histo will be used.
      numberSlicesY = (int)axis[1].size() - 1; // axis[1].size() = number of boundaries.
      useSlicingY = true;                      // Enable the use of custom boundaries.
    } else {
      ILOG(Info, Support) << "Not enough axis boundaries for slicing on Y. Will use full histogram range along Y." << ENDM;
    }
  }

  ILOG(Info, Support) << "Number of input histograms for the trending of "
                      << obj->GetName() << ": " << numberPads << ENDM;

  // Access the histograms embedded in 'obj'.
  for (int iPad = 0; iPad < numberPads; iPad++) {
    if (isCanvas) {
      auto pad = static_cast<TPad*>(padList->At(iPad));
      histo = static_cast<TH2*>(pad->GetListOfPrimitives()->At(0));
    } else { // 'obj' is already a single histogram.
      histo = static_cast<TH2*>(obj);
    }

    if (histo) {
      // Get the trending quantities defined in 'SlicerInfo'.
      // The two for-loop do only one pass if we have an input canvas.
      for (int iX = 0; iX < numberSlicesX; iX++) {
        std::string thisRange;
        if (useSlicingX) {
          histo->GetXaxis()->SetRangeUser(axis[0][iX], axis[0][iX + 1]);
          thisRange = Form("RangeX: [%.1f, %.1f]", axis[0][iX], axis[0][iX + 1]);
        } else {
          if (isCanvas) {
            thisRange = Form("ROC: %d", iPad);
          } else {
            thisRange = Form("RangeX (default): [%.1f, %.1f]", histo->GetXaxis()->GetXmin(), histo->GetXaxis()->GetXmax());
          }
        }

        for (int jY = 0; jY < numberSlicesY; jY++) {
          if (useSlicingY) {
            histo->GetYaxis()->SetRangeUser(axis[1][jY], axis[1][jY + 1]);
            // thisRange += Form(" and RangeY: [%.1f, %.1f]", axis[1][jY], axis[1][jY + 1]);
          } else {
            // thisRange += Form(" and RangeY (default): [%.1f, %.1f]", histo->GetYaxis()->GetXmin(), histo->GetYaxis()->GetXmax());
            // thisRange += Form(" and RangeY (default): [-2., 2.]");
          }
          ranges.push_back(thisRange);

          SliceInfo mySlice;
          mySlice.entries = histo->GetEntries();
          mySlice.meanX = histo->GetMean(1);
          mySlice.stddevX = histo->GetStdDev(1);
          if (mySlice.entries != 0) {
            mySlice.errMeanX = mySlice.stddevX / (sqrt(mySlice.entries));
          } else {
            mySlice.errMeanX = 0.;
          }

          mySlice.meanY = histo->GetMean(2);
          mySlice.stddevY = histo->GetStdDev(2);
          if (mySlice.entries != 0) {
            mySlice.errMeanY = mySlice.stddevY / (sqrt(mySlice.entries));
          } else {
            mySlice.errMeanY = 0.;
          }

          reducedSource.emplace_back(mySlice);
        }
      }

    } else {
      ILOG(Error, Support) << "Error: 'histo' not found." << ENDM;
    }
  } // All the vector elements have been updated.
}

} // namespace o2::quality_control_modules::tpc