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
/// \file    UserInputOutput.h
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_USERINPUTOUTPUT_H
#define QUALITYCONTROL_USERINPUTOUTPUT_H

#include <string>

#include <Framework/ConcreteDataMatcher.h>
#include <Framework/InputSpec.h>
#include <Framework/OutputSpec.h>

#include "QualityControl/DataHeaderHelpers.h"
#include "QualityControl/DataSourceType.h"

namespace o2::quality_control::core
{

/// \brief returns a standard ConcreteDataMatcher for QC inputs and outputs
framework::ConcreteDataMatcher
  createUserDataMatcher(DataSourceType dataSourceType, const std::string& detectorName, const std::string& userCodeName);

/// \brief returns a standard InputSpec for QC inputs and outputs
framework::InputSpec
  createUserInputSpec(DataSourceType dataSourceType, const std::string& detectorName, const std::string& userCodeName);

/// \brief returns a standard OutputSpec for QC inputs and outputs
framework::OutputSpec
  createUserOutputSpec(DataSourceType dataSourceType, const std::string& detectorName, const std::string& userCodeName);

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_USERINPUTOUTPUT_H