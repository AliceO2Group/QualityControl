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
/// \file   ActorHelpers.h
/// \author Piotr Konopka
///

#ifndef ACTORFACTORY_H
#define ACTORFACTORY_H

#include <ranges>

#include "QualityControl/ActorTraits.h"
#include "QualityControl/ServicesConfig.h"
#include "QualityControl/UserCodeConfig.h"
#include "QualityControl/UserCodeCardinality.h"
#include "QualityControl/Criticality.h"
#include "QualityControl/ServiceRequest.h"

namespace o2::quality_control::core
{

struct CommonSpec;

namespace actor_helpers
{

/// \brief extracts common services configuration from CommonSpec
ServicesConfig extractConfig(const CommonSpec& commonSpec);

/// \brief checks if concrete Actor requests Service S
template <typename ConcreteActor, ServiceRequest S>
consteval bool requiresService()
{
  using traits = ActorTraits<ConcreteActor>;
  // todo: when we can use C++23: std::ranges::contains(ActorTraitsT::sRequiredServices, S);
  for (const auto& required : traits::sRequiredServices) {
    if (required == S) {
      return true;
    }
  }
  return false;
}

/// \brief checks if an Actor is effectively a Runner as well, i.e. runs user code
template <typename ConcreteActor>
constexpr bool runsUserCode()
{
  using traits = ActorTraits<ConcreteActor>;
  return traits::sUserCodeCardinality != UserCodeCardinality::None;
}

/// \brief checks if an Actor is allowed to publish a given data source type
template <typename ConcreteActor>
consteval bool publishesDataSource(DataSourceType dataSourceType)
{
  using traits = ActorTraits<ConcreteActor>;
  for (auto t : traits::sPublishedDataSources) {
    if (t == dataSourceType) {
      return true;
    }
  }
  return false;
}

/// \brief checks if an Actor is allowed to publish a given data source type
template <typename ConcreteActor, DataSourceType dataSourceType>
concept ValidDataSourceForActor = publishesDataSource<ConcreteActor>(dataSourceType);

} // namespace actor_helpers

} // namespace o2::quality_control::core

#endif // ACTORFACTORY_H
