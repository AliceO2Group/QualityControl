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

#ifndef QC_INPUT_UTILS_H
#define QC_INPUT_UTILS_H

//fixme: propose a more appropriate name for the header

// std
#include <vector>
#include <string>
// o2
#include <Framework/DataProcessorSpec.h>
#include <Framework/DataSpecUtils.h>

#include "QualityControl/HashDataDescription.h"
#include "QualityControl/ActorTraits.h"

namespace o2::quality_control::core {
// the returned value can be used either as OutputSpec or InputSpec, depending who needs it
template<typename ConcreteActor, DataSourceType dataSourceType>
  requires ValidDataSourceForActor<ConcreteActor, dataSourceType>
static framework::ConcreteDataMatcher createUserDataMatcher(const std::string& detectorName,
                                                            const std::string& userCodeName)
{
  using traits = ActorTraits<ConcreteActor>;
  return {
    createDataOrigin(CharIdFrom(dataSourceType), detectorName),
    createDataDescription(userCodeName, traits::sDataDescriptionHashLength),
    0
  };
}

template<typename ConcreteActor, DataSourceType dataSourceType>
  requires ValidDataSourceForActor<ConcreteActor, dataSourceType>
static framework::OutputSpec createUserOutputSpec(const std::string& detectorName, const std::string& userCodeName)
{
  // currently all of our outputs are Lifetime::Sporadic, so we don't allow for customization, but it could be factored out
  // for a similar reason, we can safely assuming using `userCodeName` as a binding in all cases
  return {
    framework::OutputLabel{ userCodeName },
    createUserDataMatcher<ConcreteActor, dataSourceType>(detectorName, userCodeName),
    framework::Lifetime::Sporadic
  };
}

template<typename ConcreteActor, DataSourceType dataSourceType>
  requires ValidDataSourceForActor<ConcreteActor, dataSourceType>
static framework::InputSpec createUserInputSpec(const std::string& detectorName, const std::string& userCodeName)
{
  // currently all of our outputs are Lifetime::Sporadic, so we don't allow for customization, but it could be factored out
  // for a similar reason, we can safely assuming using `userCodeName` as a binding in all cases
  return {
    userCodeName,
    createUserDataMatcher<ConcreteActor, dataSourceType>(detectorName, userCodeName),
    framework::Lifetime::Sporadic
  };
}

// fixme: rename to stringifyInputs?
inline std::vector<std::string> stringifyInput(const o2::framework::Inputs& inputs)
{
  std::vector<std::string> vec;
  for (const auto& input : inputs) {
    vec.push_back(o2::framework::DataSpecUtils::describe(input));
  }
  return vec;
}
}
#endif
