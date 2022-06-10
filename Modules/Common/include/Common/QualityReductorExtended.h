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
/// \file     QualityReductorExtended.h
/// \author   Marcel Lesch
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_QUALITYREDUCTOREXTENDED_H
#define QUALITYCONTROL_QUALITYREDUCTOREXTENDED_H

#include "QualityControl/ReductorExtended.h"
#include "QualityControl/SliceInfoTrending.h"

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::common
{
/// \brief A reductor of quality objects for the extended trending.
///
/// A Reductor for the quality objects. It receives
/// a struct of 'SliceInfoQuality'
///

class QualityReductorExtended : public quality_control::postprocessing::ReductorExtended
{
 public:
  /// \brief Constructor.
  QualityReductorExtended() = default;
  /// \brief Destructor.
  ~QualityReductorExtended() = default;

  /// \brief Methods from the extended reductor class.
  void updateQuality(const TObject* obj, SliceInfoQuality& reducedQualitySource) final;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_QUALITYREDUCTOREXTENDED_H
