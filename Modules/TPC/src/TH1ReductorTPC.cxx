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
/// \file   TH1Reductor.cxx
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///

#include "TPC/TH1ReductorTPC.h"
#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>
#include <vector>
#include <TCanvas.h>
#include <TList.h>
#include <TAxis.h>

namespace o2::quality_control_modules::tpc
{
/*
void* TH1ReductorTPC::getBranchAddress()
{
  //return &mStats;
  return Form("Hellow");
}

const char* TH1ReductorTPC::getBranchLeafList()
{
  //return Form("numberPads/I:entries[%i]/D:mean[%i]:stddev[%i]", numberPads, numberPads, numberPads);
  return Form("Hellow");
}
*/
void TH1ReductorTPC::update(TObject* obj, std::vector<SliceInfo>& reducedSource, std::vector<std::vector<float>>& axis)
{
  TList* padList = nullptr;   // List of TPads for input TCanvas.
  TH1* histo = nullptr;       // General pointer to the histogram to trend.
  int numberPads = 1;         // Default value if no slicer and not a canvas.
  int axisSize = 1;           // Default value for the main slicer axis (for the canvas as input).
  int innerAxisSize = 2;      // Default value for the inner slicer axis (for the canvas as input).

  ILOG(Info, Support) << "Entering Reductor::update." << ENDM;

  // Get the number of pads (and list in case of a canvas).
  const bool isCanvas = (obj->IsA() == TCanvas::Class());
  if (isCanvas) {
    auto canvas = static_cast<TCanvas*>(obj);
    padList = static_cast<TList*>(canvas->GetListOfPrimitives());
    padList->SetOwner(kTRUE);
    numberPads = padList->GetEntries();
  } else {  // Slicer case for one histo: number of pads = axis.size*innerAxis.size.
    if (axis[0].size() > 1) { // To ensure we use the slicing.
      axisSize = (int)axis.size();
      innerAxisSize = (int)axis[0].size() - 1;
      numberPads = axisSize*innerAxisSize;
    }
  }
  ILOG(Info, Support) << "Trending of " << obj->GetName() << " will contain " << numberPads << " histogram(s)." << ENDM;

  // Access the histo embedded in obj.
  for (int iPad = 0; iPad < numberPads; iPad++) {
    if (isCanvas) {
      auto pad = static_cast<TPad*>(padList->At(iPad));
      histo = static_cast<TH1*>(pad->GetListOfPrimitives()->At(0));
    } else { // The passed "obj" is directly an histogram.
      histo = static_cast<TH1*>(obj);
    }

  // Get the trending quantities.
    if (histo) {
      for (int i = 0; i < axisSize; i++) {  // 1 pass if canvas.
        for (int j = 0; j < innerAxisSize; j++) { // 1 pass if canvas.
          int index = iPad; // array index in case of canvas.
          if (!isCanvas) {
            index = j;
            if (innerAxisSize > 1) {  // Cut the axes if we have enough slices.
              histo->GetXaxis()->SetRangeUser(axis[i][j], axis[i][j + 1]);
            }
          }

          SliceInfo mySlice;
          mySlice.entries = histo->GetEntries();
          mySlice.meanX = histo->GetMean(1);//-1.;  // Not used if slicing.
          mySlice.stddevX = histo->GetStdDev(1);//-1.;  // Not used if slicing.
          mySlice.errMeanX = mySlice.stddevX/(sqrt(mySlice.entries));//-1.;  // Not used if slicing.
          mySlice.meanY = histo->GetMean(2);
          mySlice.stddevY = histo->GetStdDev(2);
          mySlice.errMeanY = mySlice.stddevY/(sqrt(mySlice.entries));
          if (isCanvas) {
            mySlice.meanX = histo->GetMean(1);
            mySlice.stddevX = histo->GetStdDev(1);
            mySlice.errMeanX = mySlice.stddevX/(sqrt(mySlice.entries));
          }

          reducedSource.emplace_back(mySlice);
          ILOG(Info, Support) << "i: " << i << " Index: " << index << " Mean slice along x: "
            << mySlice.meanX << " Mean slice along y: " << mySlice.meanY << ENDM;
        }
      }
    } else {
      ILOG(Error, Support) << "Error: histo not found." << ENDM;
    }
  }
}

} // namespace o2::quality_control_modules::tpc