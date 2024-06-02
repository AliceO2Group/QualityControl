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
/// \file   ObjectComparatorKolmogorov.h
/// \author Andrea Ferrero
/// \brief  A class for comparing two histogram objects based on a Kolmogorov test
///

#ifndef QUALITYCONTROL_ObjectComparatorKolmogorov_H
#define QUALITYCONTROL_ObjectComparatorKolmogorov_H

#include "Common/ObjectComparatorInterface.h"

namespace o2::quality_control_modules::common
{

/// \brief A class for comparing two histogram objects based on a Kolmogorov test
class ObjectComparatorKolmogorov : public ObjectComparatorInterface
{
 public:
  /// \brief Constructor
  ObjectComparatorKolmogorov() = default;
  /// \brief Destructor
  virtual ~ObjectComparatorKolmogorov() = default;

  /// \brief objects comparison function
  /// \return the quality resulting from the object comparison
  o2::quality_control::core::Quality compare(TObject* obj, TObject* objRef, std::string& message) override;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_ObjectComparatorKolmogorov_H
