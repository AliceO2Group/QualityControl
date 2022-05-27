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
/// \file     TH2ReductorTPC.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TH2REDUCTORTPC_H
#define QUALITYCONTROL_TH2REDUCTORTPC_H

#include "TPC/ReductorTPC.h"

namespace o2::quality_control_modules::tpc
{
/// \brief A reductor of TH2 histograms for the trending of TPC objects.
///
/// A Reductor for the TH2 histograms used for the TPC quantities. It receives
/// a vector of 'SliceInfo' which size corresponds to the number of slices or
/// pads which need to be trended.
///

class TH2ReductorTPC : public quality_control_modules::tpc::ReductorTPC
{
 public:
  /// \brief Constructor.
  TH2ReductorTPC() = default;
  /// \brief Destructor.
  ~TH2ReductorTPC() = default;

  /// \brief Methods from the reductor class adapted for the needs of the TPC.
  void update(TObject* obj, std::vector<SliceInfo>& reducedSource,
              std::vector<std::vector<float>>& axis, std::vector<std::string>& ranges) final;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_TH2REDUCTORTPC_H
