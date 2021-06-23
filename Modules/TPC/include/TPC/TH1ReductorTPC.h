// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

#include "TPC/ReductorTPC.h"
#include <vector>

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor which obtains the most popular characteristics of TH1.
///
/// A Reductor which obtains the most popular characteristics of TH1.
/// It produces a branch in the format: "mean/D:stddev:entries".
class TH1ReductorTPC : public quality_control_modules::tpc::ReductorTPC
{
 public:
  /// \brief Constructor.
  TH1ReductorTPC() = default;
  /// \brief Destructor.
  ~TH1ReductorTPC() = default;

  /// \brief Definitions of the methods common to all reductors.
  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj, std::vector<std::vector<float>>& axis, bool isCanvas) override;

 private:
  static constexpr int NMAXSLICES = 72; ///< Maximum number of slices, or histograms for an input canvas.

  struct {
    Double_t mean[NMAXSLICES];    ///< Mean value for each slice/histogram.
    Double_t stddev[NMAXSLICES];  ///< Standard deviation for each slice/histogram.
    Double_t entries[NMAXSLICES]; ///< Number of entries for each slice/histogram.
  } mStats;
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_TH1REDUCTORTPC_H
