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
/// \file   MeanVertexValidator.h
/// \author Andrea Ferrero
///
#ifndef QUALITYCONTROL_MEANVERTEX_VALIDATOR_H
#define QUALITYCONTROL_MEANVERTEX_VALIDATOR_H

#include "QualityControl/CcdbValidatorInterface.h"

namespace o2::quality_control_modules::glo
{

/// \brief Inspect and validate the contents of MeanVertex calibration objects
class MeanVertexValidator : public o2::quality_control::postprocessing::CcdbValidatorInterface
{
 public:
  /// \brief Constructor
  MeanVertexValidator() = default;
  /// \brief Destructor
  virtual ~MeanVertexValidator() override = default;

  const std::type_info& getTinfo() const override;

  /// \brief Fill the data structure with new data
  /// \param An object to be reduced into a limited set of observables
  bool validate(void* obj) override;
};

} // namespace o2::quality_control_modules::glo
#endif // QUALITYCONTROL_MEANVERTEX_VALIDATOR_H
