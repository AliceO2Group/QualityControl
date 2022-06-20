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
/// \file     SliceReductor.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_SLICEREDUCTOR_H
#define QUALITYCONTROL_SLICEREDUCTOR_H

#include "QualityControl/SliceInfoTrending.h"
#include <TObject.h>
#include <vector>
#include "TAxis.h"

namespace o2::quality_control::postprocessing
{
/// \brief An interface for storing data derived from QC objects into a TTree.
///
/// A extended reductor class from which each reductor used for the extended trending.
///

class SliceReductor
{
 public:
  /// \brief Constructor.
  SliceReductor() = default;
  /// \brief Destructor.
  virtual ~SliceReductor() = default;

  /// \brief Methods from the reductor class adapted for the needs of the TPC.
  virtual void update(TObject* obj, std::vector<SliceInfo>& reducedSource,
                      std::vector<std::vector<float>>& axis,
                      int& finalNumberPads){};

  /// \brief Function to return proper bin numbers to avoid double counting if slicing is used
  void getBinSlices(TAxis* histAxis, const float sliceLow, const float sliceUp, int& binLow, int& binUp, float& sliceLabel)
  {
    binLow = histAxis->FindBin(sliceLow);
    if (sliceLow > histAxis->GetBinCenter(binLow)) {
      binLow += 1;
    } // Lower slice boundary is above bin center. Start at next higher bin
    binUp = histAxis->FindBin(sliceUp);
    if (sliceUp <= histAxis->GetBinCenter(binUp)) {
      binUp -= 1;
    } // Upper slice boundary is smaller equal bin center. Stop at next lower bin

    sliceLabel = (sliceLow + sliceUp) / 2.;
  }
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_SLICEREDUCTOR_H
