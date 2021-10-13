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
/// \file   TH2ReductorTPC.h
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TH2REDUCTORTPC_H
#define QUALITYCONTROL_TH2REDUCTORTPC_H

#include "TPC/SliceInfo.h"
#include "TPC/ReductorTPC.h"
#include <vector>

namespace o2::quality_control_modules::tpc
{

/// \brief A TPC-specific reductor which obtains the most popular characteristics of TH2.
///
/// A Reductor which obtains the most popular characteristics of TH2, in the case of TPC-related quantities.
/// It produces a branch in the format: "mean/D:stddev:entries". All these variables are saved as arrays of doubles with a maximum length of 72 (i.e. number of ROCs in the full TPC).
class TH2ReductorTPC : public quality_control_modules::tpc::ReductorTPC
{
 public:
  /// \brief Constructor.
  TH2ReductorTPC() = default;
  /// \brief Destructor.
  ~TH2ReductorTPC() = default;

  /// \brief Definitions of the methods common to all reductors.
  //void* getBranchAddress() final;
  //const char* getBranchLeafList() final;
  void update(TObject* obj, std::vector<SliceInfo>& reducedSource, std::vector<std::vector<float>>& axis) final;

 private:
  static constexpr int NMAXSLICES = 72; ///< Maximum number of slices, or histograms for an input canvas. TBI: Make configurable.

  struct { ///< All quantities of TH2 for each x-slice/histogram.
    Double_t sumw[NMAXSLICES];
    Double_t sumw2[NMAXSLICES];
    Double_t sumwx[NMAXSLICES];
    Double_t sumwx2[NMAXSLICES];
    Double_t sumwy[NMAXSLICES];
    Double_t sumwy2[NMAXSLICES];
    Double_t sumwxy[NMAXSLICES];
    Double_t entries[NMAXSLICES]; // Is sumw == entries always? maybe not for values which land into the edge bins?
  } mStats;
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_TH2REDUCTORTPC_H
