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

          mySlice.meanY = histo->GetMean(2);
          mySlice.stddevY = histo->GetStdDev(2);
          mySlice.errMeanY = mySlice.stddevY / (sqrt(mySlice.entries));
          /*
          if (isCanvas) {
            mySlice.meanX = histo->GetMean(1);
            mySlice.stddevX = histo->GetStdDev(1);
            mySlice.errMeanX = mySlice.stddevX/(sqrt(mySlice.entries));
          }*/

          reducedSource.emplace_back(mySlice);
          /*ILOG(Info, Support) << "i: " << i << " Index: " << index << " Mean slice along x: "
            << mySlice.meanX << " Mean slice along y: " << mySlice.meanY << ENDM;*/
        }
      }

    } else {
      ILOG(Error, Support) << "Error: 'histo' not found." << ENDM;
    }
  } // All the vector elements have been updated.
}

} // namespace o2::quality_control_modules::tpc