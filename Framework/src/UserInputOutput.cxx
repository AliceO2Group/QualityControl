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
/// \file    UserInputOutput.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/UserInputOutput.h"

namespace o2::quality_control::core
{

framework::ConcreteDataMatcher
  createUserDataMatcher(DataSourceType dataSourceType, const std::string& detectorName, const std::string& userCodeName,
                        o2::header::DataHeader::SubSpecificationType subSpec)
{
  return {
    createDataOrigin(dataSourceType, detectorName),
    createDataDescription(userCodeName, dataSourceType),
    subSpec
  };
}

framework::InputSpec
  createUserInputSpec(DataSourceType dataSourceType, const std::string& detectorName, const std::string& userCodeName,
                      o2::header::DataHeader::SubSpecificationType subSpec, const std::string& binding)
{
  // currently all of our outputs are Lifetime::Sporadic, so we don't allow for customization, but it could be factored out.
  return {
    binding.empty() ? userCodeName : binding,
    createUserDataMatcher(dataSourceType, detectorName, userCodeName, subSpec),
    framework::Lifetime::Sporadic
  };
}

framework::OutputSpec
  createUserOutputSpec(DataSourceType dataSourceType, const std::string& detectorName, const std::string& userCodeName,
                       o2::header::DataHeader::SubSpecificationType subSpec, const framework::OutputLabel& binding)
{
  // currently all of our outputs are Lifetime::Sporadic, so we don't allow for customization, but it could be factored out.
  return {
    binding.value.empty() ? framework::OutputLabel{ userCodeName } : binding,
    createUserDataMatcher(dataSourceType, detectorName, userCodeName, subSpec),
    framework::Lifetime::Sporadic
  };
}

} // namespace o2::quality_control::core
