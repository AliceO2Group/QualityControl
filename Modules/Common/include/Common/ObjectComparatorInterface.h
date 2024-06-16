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
/// \file   ObjectComparatorInterface.h
/// \author Andrea Ferrero
/// \brief  An interface for comparing two TObject
///

#ifndef QUALITYCONTROL_ObjectComparatorInterface_H
#define QUALITYCONTROL_ObjectComparatorInterface_H

#include "QualityControl/Quality.h"
#include "QualityControl/CustomParameters.h"
#include "QualityControl/Activity.h"

#include <tuple>

class TObject;
class TH1;

namespace o2::quality_control_modules::common
{

/// \brief An interface for comparing two TObject
class ObjectComparatorInterface
{
 public:
  /// \brief Constructor
  ObjectComparatorInterface() = default;
  /// \brief Destructor
  virtual ~ObjectComparatorInterface() = default;

  /// \brief comparator configuration via CustomParameters
  // virtual void configure(const o2::quality_control::core::CustomParameters& customParameters, const o2::quality_control::core::Activity activity = {}){};

  /// setter/getter methods for the threshold to define the goodness of the comparison
  void setThreshold(double threshold) { mThreshold = threshold; }
  double getThreshold() { return mThreshold; }

  /// perform a number of sanity checks on the input objects
  /// \return a tuple containing pointers to the histogram, the reference histogram, and a boolean indicating the success of the checks
  std::tuple<TH1*, TH1*, bool> checkInputObjects(TObject* object, TObject* referenceObject, std::string& message);

  /// \brief objects comparison function
  /// \return the quality resulting from the object comparison
  virtual o2::quality_control::core::Quality compare(TObject* object, TObject* referenceObject, std::string& message) = 0;

 private:
  /// the threshold to define the goodness of the comparison
  double mThreshold{ 0 };
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_ObjectComparatorInterface_H
