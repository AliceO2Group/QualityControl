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
/// \file   ObjectComparatorDeviation.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_ObjectComparatorDeviation_H
#define QUALITYCONTROL_ObjectComparatorDeviation_H

#include "Common/ObjectComparatorInterface.h"

namespace o2::quality_control_modules::common
{

/// \brief An interface for storing columnar data into a TTree
class ObjectComparatorDeviation : public ObjectComparatorInterface
{
 public:
  /// \brief Constructor
  ObjectComparatorDeviation() = default;
  /// \brief Destructor
  virtual ~ObjectComparatorDeviation() = default;

  /// \brief comparator configuration via CustomParameters
  void configure(const o2::quality_control::core::CustomParameters& customParameters, const o2::quality_control::core::Activity activity = {}) override;

  /// \brief objects comparison function
  /// \return the quality resulting from the object comparison
  o2::quality_control::core::Quality compare(TObject* obj, TObject* objRef, std::string& message) override;

 private:
  float mThreshold{ 0 };
  bool mNormalize{ false };
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_ObjectComparatorDeviation_H
