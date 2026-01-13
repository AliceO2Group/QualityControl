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
/// \file   ActorTraits.h
/// \author Piotr Konopka
///

#ifndef ACTORTRAITS_H
#define ACTORTRAITS_H

#include <string_view>
#include <ranges>

#include <Headers/DataHeader.h>
#include <BookkeepingApi/DplProcessType.h>

#include "QualityControl/DataSourceSpec.h"
#include "QualityControl/stringUtils.h"

// ActorTraits and specializations should not bring a lot of dependencies.
// They should define the expected traits for each QC Actor and basic behaviours.

namespace o2::quality_control::core {

// Enum class for instance cardinality
enum class UserCodeInstanceCardinality {
  None = 0,
  One = 1,
  Many = 2
};

// Helper to check if an Actor is effectively a Runner as well
template<typename ActorTraitsT>
constexpr bool runsUserCode() {
  return ActorTraitsT::sUserCodeInstanceCardinality != UserCodeInstanceCardinality::None;
}

// could be moved to another header
enum class Service {
  Monitoring,
  InfoLogger,
  CCDB,
  Bookkeeping,
  QCDB
};

template<typename ActorTraitsT, Service S>
consteval bool requiresService() {
  for (const auto& required : ActorTraitsT::sRequiredServices) {
    if (required == S) {
      return true;
    }
  }
  return false;
}

enum class Criticality
{
  Resilient,
  Critical,
  Expendable,
  UserDefined // user says if it's critical or expendable
};

template<typename ActorTraitsT>
concept ValidActorTraits = requires
{
  // must have the following static constants

  // names in different forms for use in registering the actor in different services, etc...
  { ActorTraitsT::sActorTypeShort } -> std::convertible_to<std::string_view>;

  { ActorTraitsT::sActorTypeKebabCase } -> std::convertible_to<std::string_view>;
  requires isKebabCase(ActorTraitsT::sActorTypeKebabCase);

  { ActorTraitsT::sActorTypeUpperCamelCase } -> std::convertible_to<std::string_view>;
  requires isUpperCamelCase(ActorTraitsT::sActorTypeUpperCamelCase);

  // supported inputs and outputs by a given actor
  { ActorTraitsT::sConsumedDataSources } -> std::ranges::input_range;
  requires std::convertible_to<std::ranges::range_value_t<decltype(ActorTraitsT::sConsumedDataSources)>, DataSourceType>;

  { ActorTraitsT::sPublishedDataSources } -> std::ranges::input_range;
  requires std::convertible_to<std::ranges::range_value_t<decltype(ActorTraitsT::sPublishedDataSources)>, DataSourceType>;

  // a list of required services, Actor will take care of initializing them
  { ActorTraitsT::sRequiredServices } -> std::ranges::input_range;
  requires std::convertible_to<std::ranges::range_value_t<decltype(ActorTraitsT::sRequiredServices)>, Service>;
  // for certain services, we require additional fields
  requires (requiresService<ActorTraitsT, Service::Bookkeeping>() ?
            requires { { ActorTraitsT::sDplProcessType } -> std::convertible_to<o2::bkp::DplProcessType>; } :
            true);


  // we want to know if this Actor runs user code.
  // fixme: now could be simplified to a bool, but maybe some future usage will need One/Many distinction
  { ActorTraitsT::sUserCodeInstanceCardinality } -> std::convertible_to<UserCodeInstanceCardinality>;

  // do we normally associate this Actor with a specific detector (in the worst case, with "MANY" or "MISC")
  { ActorTraitsT::sDetectorSpecific } -> std::convertible_to<bool>;

  // specifies how an actor should be treated by a control system if it crashes
  { ActorTraitsT::sCriticality } -> std::convertible_to<Criticality>;

  // used to create data description when provided strings are too long
  { ActorTraitsT::sDataDescriptionHashLength } -> std::convertible_to<size_t>;
  requires (ActorTraitsT::sDataDescriptionHashLength <= o2::header::DataDescription::size);

  // todo: a constant to set how actor consumes inputs, i.e. how to customize CompletionPolicy
};

// this is a fallback struct which is activated only if a proper specialization is missing (SFINAE).
template<typename ActorType>
struct ActorTraits {
  static_assert(false, "ActorTraits must be specialized for each actor type.");
};

template<typename ConcreteActor>
consteval bool publishesDataSource(DataSourceType dataSourceType)
{
  for (auto t : ActorTraits<ConcreteActor>::sPublishedDataSources) {
    if (t == dataSourceType) {
      return true;
    }
  }
  return false;
}

template<typename ConcreteActor, DataSourceType dataSourceType>
concept ValidDataSourceForActor =
  publishesDataSource<ConcreteActor>(dataSourceType);

}
#endif //ACTORTRAITS_H
