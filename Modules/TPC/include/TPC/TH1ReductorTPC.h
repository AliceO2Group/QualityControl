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
/// \file     TH1ReductorTPC.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TH1REDUCTORTPC_H
#define QUALITYCONTROL_TH1REDUCTORTPC_H

#include "TPC/ReductorTPC.h"
#include "TPC/SliceInfo.h"
#include <vector>
#include "TH1.h"

namespace o2::quality_control_modules::tpc
{
/// \brief A reductor of TH1 histograms for the trending of TPC objects.
///
/// A Reductor for the TH1 histograms used for the TPC quantities. It receives
/// a vector of 'SliceInfo' which size corresponds to the number of slices or
/// pads which need to be trended.
///
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka

class TH1ReductorTPC : public quality_control_modules::tpc::ReductorTPC
{
 public:
  /// \brief Constructor.
  TH1ReductorTPC() = default;
  /// \brief Destructor.
  ~TH1ReductorTPC() = default;

  /// \brief Methods from the reductor class adapted for the needs of the TPC.
  void update(TObject* obj, std::vector<SliceInfo>& reducedSource,
              std::vector<std::vector<float>>& axis) final;

 private:
  void GetTH1StatsY(TH1* Hist, float Stats[3], float LowerBoundary, float UpperBoundary);
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_TH1REDUCTORTPC_H