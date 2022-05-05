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
/// \file     ReductorTPC.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_REDUCTORTPC_H
#define QUALITYCONTROL_REDUCTORTPC_H

#include "TPC/SliceInfo.h"
#include <TObject.h>
#include <vector>
#include "TAxis.h"

namespace o2::quality_control_modules::tpc
{
/// \brief An interface for storing data derived from QC objects into a TTree.
///
/// A TPC-specific reductor class from which each reductor used for the trending
/// of the TPC-related quantities inherit.
///

class ReductorTPC
{
 public:
  /// \brief Constructor.
  ReductorTPC() = default;
  /// \brief Destructor.
  virtual ~ReductorTPC() = default;

  /// \brief Methods from the reductor class adapted for the needs of the TPC.
  virtual void update(TObject* obj, std::vector<SliceInfo>& reducedSource,
                      std::vector<std::vector<float>>& axis,
                      std::vector<std::string>& ranges){};

  /// \brief Methods from the reductor class adapted for the needs of the TPC QO.
  virtual void updateQuality(const TObject* obj, SliceInfoQuality& reducedSource, std::vector<std::string>& ranges){};

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

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_REDUCTORTPC_H
