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
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_QUALITYREDUCTORTPC_H
#define QUALITYCONTROL_QUALITYREDUCTORTPC_H

#include "TPC/ReductorTPC.h"

namespace o2::quality_control_modules::tpc
{
/// \brief A reductor of quality objects for the trending of TPC objects.
///
/// A Reductor for the quality objects used for the TPC quantities. It receives
/// a struct of 'SliceInfoQuality'
///

class QualityReductorTPC : public quality_control_modules::tpc::ReductorTPC
{
 public:
  /// \brief Constructor.
  QualityReductorTPC() = default;
  /// \brief Destructor.
  ~QualityReductorTPC() = default;

  /// \brief Methods from the reductor class adapted for the needs of the TPC.
  void updateQuality(const TObject* obj, SliceInfoQuality& reducedQualitySource, std::vector<std::string>& ranges) final;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_QUALITYREDUCTORTPC_H
