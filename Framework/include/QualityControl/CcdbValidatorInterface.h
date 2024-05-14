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
/// \file   CcdbValidatorInterface.h
/// \author Andrea Ferrero
///
#ifndef QUALITYCONTROL_CCDB_VALIDATOR_INTERFACE_H
#define QUALITYCONTROL_CCDB_VALIDATOR_INTERFACE_H

#include <typeinfo>

namespace o2::quality_control::postprocessing
{

/// \brief An interface for validating the content of CCDB objects
class CcdbValidatorInterface
{
 public:
  /// \brief Constructor
  CcdbValidatorInterface() = default;
  /// \brief Destructor
  virtual ~CcdbValidatorInterface() = default;

  /// \brief return the type_info of the CCDB object to be inspected
  virtual const std::type_info& getTinfo() const = 0;

  /// \brief Inspect and validate the contants of a given CCDB object
  /// \param An object to be inspected
  virtual bool validate(void* obj) = 0;
};

} // namespace o2::quality_control::postprocessing
#endif // QUALITYCONTROL_CCDB_VALIDATOR_INTERFACE_H
