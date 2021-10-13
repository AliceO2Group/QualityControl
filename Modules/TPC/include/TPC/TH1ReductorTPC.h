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
/// \file   TH1ReductorTPC.h
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TH1REDUCTORTPC_H
#define QUALITYCONTROL_TH1REDUCTORTPC_H

#include "TPC/SliceInfo.h"
#include "TPC/ReductorTPC.h"
#include <vector>

namespace o2::quality_control_modules::tpc
{
  /*
  struct sliceInfo {
    //sliceInfo() = default;
    //~sliceInfo() = default;

    double entries;   ///< Number of entries for each slice/histogram.
    double mean;      ///< Mean value for each slice/histogram.
    double stddev;    ///< Standard deviation for each slice/histogram.
  };*/

/// \brief A TPC-specific reductor which obtains the most popular characteristics of TH1.
///
/// A Reductor which obtains the most popular characteristics of TH1, in the case of TPC-related quantities.
/// It produces a branch containing a vector of "SliceInfo" which contain all characteristic of TH1.
/// Vectors are used as it allows to have input canvas with any number of pads on it.
class TH1ReductorTPC : public quality_control_modules::tpc::ReductorTPC
{
 public:
  /// \brief Constructor.
  TH1ReductorTPC() = default;
  /// \brief Destructor.
  ~TH1ReductorTPC() = default;
/*
  /// \brief Definitions of the methods common to all reductors for TPC.
  void* getBranchAddress() final;
  const char* getBranchLeafList() final;*/
  void update(TObject* obj, std::vector<SliceInfo>& reducedSource, std::vector<std::vector<float>>& axis) final;

 //private:
  /*struct sliceInfo {
    //sliceInfo() = default;
    //~sliceInfo() = default;

    double entries;   ///< Number of entries for each slice/histogram.
    double mean;      ///< Mean value for each slice/histogram.
    double stddev;    ///< Standard deviation for each slice/histogram.
  };
  */
  //std::vector<sliceInfo> mStats;  //!<!

};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_TH1REDUCTORTPC_H