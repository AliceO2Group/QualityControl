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
/// \file     TH2ReductorExtended.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TH2REDUCTOREXTENDED_H
#define QUALITYCONTROL_TH2REDUCTOREXTENDED_H

#include "QualityControl/ReductorExtended.h"
#include "QualityControl/SliceInfoTrending.h"

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::common
{
/// \brief A reductor of TH2 histograms for the extended trending of objects.
///
/// A Reductor for the TH2 histograms. It receives
/// a vector of 'SliceInfo' which size corresponds to the number of slices or
/// pads which need to be trended.
///

class TH2ReductorExtended : public quality_control::postprocessing::ReductorExtended
{
 public:
  /// \brief Constructor.
  TH2ReductorExtended() = default;
  /// \brief Destructor.
  ~TH2ReductorExtended() = default;

  /// \brief Methods from the extended reductor class.
  void update(TObject* obj, std::vector<SliceInfo>& reducedSource,
              std::vector<std::vector<float>>& axis, int& finalNumberPads) final;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_TH2REDUCTOREXTENDED_H
