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
/// \file    testActorTraits.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/ActorTraits.h"
#include <catch_amalgamated.hpp>

namespace o2::quality_control::core
{

struct CorrectActor;
template <>
struct ActorTraits<CorrectActor> {
  constexpr static std::string_view sActorTypeShort{ "wheel" };
  constexpr static std::string_view sActorTypeKebabCase{ "qc-wheel-runer" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "WheelRunner" };
  constexpr static size_t sDataDescriptionHashLength{ 4 };
  constexpr static std::array<DataSourceType, 1> sConsumedDataSources{ DataSourceType::Task };
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources{ DataSourceType::Check };
  constexpr static std::array<ServiceRequest, 3> sRequiredServices{ ServiceRequest::InfoLogger, ServiceRequest::Monitoring };
  constexpr static o2::bkp::DplProcessType sDplProcessType{ o2::bkp::DplProcessType::MERGER };
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::One };
  constexpr static bool sDetectorSpecific{ true };
  constexpr static Criticality sCriticality{ Criticality::UserDefined };
};
using CorrectActorTraits = ActorTraits<CorrectActor>;

struct WrongActorA;
template <>
struct ActorTraits<WrongActorA> {
  constexpr static std::string_view sActorTypeShort{ "wheel" };
  constexpr static std::string_view sActorTypeKebabCase{ "qc-wheel-runer" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "WheelRunner" };
  constexpr static size_t sDataDescriptionHashLength{ 4 };
  constexpr static std::array<DataSourceType, 1> sConsumedDataSources{ DataSourceType::Task };
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources{ DataSourceType::Check };
  constexpr static std::array<ServiceRequest, 3> sRequiredServices{ ServiceRequest::InfoLogger, ServiceRequest::Monitoring, ServiceRequest::Bookkeeping };
  // <---- missing o2::bkp::DplProcessType sDplProcessType
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::One };
  constexpr static bool sDetectorSpecific{ true };
  constexpr static Criticality sCriticality{ Criticality::UserDefined };
};
using MissingDplProcessTypeForBKPTraits = ActorTraits<WrongActorA>;

struct WrongActorB;
template <>
struct ActorTraits<WrongActorB> {
};
using EmptyActorTraits = ActorTraits<WrongActorB>;

struct WrongActorC;
template <>
struct ActorTraits<WrongActorC> {
  std::string_view sActorTypeShort{ "wheel" }; // <---- wrong
  constexpr static std::string_view sActorTypeKebabCase{ "qc-wheel-runer" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "WheelRunner" };
  constexpr static size_t sDataDescriptionHashLength{ 4 };
  constexpr static std::array<DataSourceType, 1> sConsumedDataSources{ DataSourceType::Task };
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources{ DataSourceType::Check };
  constexpr static std::array<ServiceRequest, 3> sRequiredServices{ ServiceRequest::InfoLogger, ServiceRequest::Monitoring, ServiceRequest::Bookkeeping };
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::One };
  constexpr static bool sDetectorSpecific{ true };
  constexpr static Criticality sCriticality{ Criticality::UserDefined };
};
using NonConstStaticActorTraits = ActorTraits<WrongActorC>;

struct WrongActorD;
template <>
struct ActorTraits<WrongActorD> {
  std::string_view sActorTypeShort{ "wheel" };
  constexpr static std::string_view sActorTypeKebabCase{ "qc-wheel-runer" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "WheelRunner" };
  constexpr static size_t sDataDescriptionHashLength{ o2::header::DataDescription::size + 555 }; // <---- wrong
  constexpr static std::array<DataSourceType, 1> sConsumedDataSources{ DataSourceType::Task };
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources{ DataSourceType::Check };
  constexpr static std::array<ServiceRequest, 3> sRequiredServices{ ServiceRequest::InfoLogger, ServiceRequest::Monitoring, ServiceRequest::Bookkeeping };
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::One };
  constexpr static bool sDetectorSpecific{ true };
  constexpr static Criticality sCriticality{ Criticality::UserDefined };
};
using TooLongDataDescriptionHashTraits = ActorTraits<WrongActorD>;

struct WrongActorE;
template <>
struct ActorTraits<WrongActorE> {
  std::string_view sActorTypeShort{ "wheel" };
  constexpr static std::string_view sActorTypeKebabCase{ "WheelRunner" }; // <---- wrong
  constexpr static std::string_view sActorTypeUpperCamelCase{ "WheelRunner" };
  constexpr static size_t sDataDescriptionHashLength{ o2::header::DataDescription::size + 555 };
  constexpr static std::array<DataSourceType, 1> sConsumedDataSources{ DataSourceType::Task };
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources{ DataSourceType::Check };
  constexpr static std::array<ServiceRequest, 3> sRequiredServices{ ServiceRequest::InfoLogger, ServiceRequest::Monitoring, ServiceRequest::Bookkeeping };
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::One };
  constexpr static bool sDetectorSpecific{ true };
  constexpr static Criticality sCriticality{ Criticality::UserDefined };
};
using KebabCaseTypeNotRespected = ActorTraits<WrongActorE>;

struct WrongActorF;
template <>
struct ActorTraits<WrongActorF> {
  std::string_view sActorTypeShort{ "wheel" };
  constexpr static std::string_view sActorTypeKebabCase{ "qc-wheel-runer" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "qc-wheel-runner" }; // <---- wrong
  constexpr static size_t sDataDescriptionHashLength{ o2::header::DataDescription::size + 555 };
  constexpr static std::array<DataSourceType, 1> sConsumedDataSources{ DataSourceType::Task };
  constexpr static std::array<DataSourceType, 1> sPublishedDataSources{ DataSourceType::Check };
  constexpr static std::array<ServiceRequest, 3> sRequiredServices{ ServiceRequest::InfoLogger, ServiceRequest::Monitoring, ServiceRequest::Bookkeeping };
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::One };
  constexpr static bool sDetectorSpecific{ true };
  constexpr static Criticality sCriticality{ Criticality::UserDefined };
};
using UpperCamelCaseTypeNotRespected = ActorTraits<WrongActorF>;

TEST_CASE("valid actor traits")
{
  STATIC_CHECK(ValidActorTraits<CorrectActorTraits>);
  STATIC_CHECK(ValidActorTraits<MissingDplProcessTypeForBKPTraits> == false);
  STATIC_CHECK(ValidActorTraits<EmptyActorTraits> == false);
  STATIC_CHECK(ValidActorTraits<NonConstStaticActorTraits> == false);
  STATIC_CHECK(ValidActorTraits<TooLongDataDescriptionHashTraits> == false);
  STATIC_CHECK(ValidActorTraits<KebabCaseTypeNotRespected> == false);
  STATIC_CHECK(ValidActorTraits<UpperCamelCaseTypeNotRespected> == false);
}

} // namespace o2::quality_control::core
