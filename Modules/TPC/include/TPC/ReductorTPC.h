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

namespace o2::quality_control_modules::tpc
{
/// \brief An interface for storing data derived from QC objects into a TTree.
///
/// A TPC-specific reductor class from which each reductor used for the trending
/// of the TPC-related quantities inherit.
///
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka

class ReductorTPC
{
 public:
  /// \brief Constructor.
  ReductorTPC() = default;
  /// \brief Destructor.
  virtual ~ReductorTPC() = default;

  /// \brief Methods from the reductor class adapted for the needs of the TPC.
  virtual void update(TObject* obj, std::vector<SliceInfo>& reducedSource,
                      std::vector<std::vector<float>>& axis) = 0;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_REDUCTORTPC_H