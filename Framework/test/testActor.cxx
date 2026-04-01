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
/// \file    testActor.cxx
/// \author  Piotr Konopka
///

#include <catch_amalgamated.hpp>

#include "QualityControl/Actor.h"
#include "QualityControl/ActorTraits.h"
#include "QualityControl/ActorHelpers.h"
#include "QualityControl/ServicesConfig.h"

#include <Monitoring/Monitoring.h>

namespace o2::quality_control::core
{

struct DummyActor;

template <>
struct ActorTraits<DummyActor> {
  constexpr static std::string_view sActorTypeShort{ "dummy" };
  constexpr static std::string_view sActorTypeKebabCase{ "qc-dummy-actor" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "DummyActor" };
  constexpr static size_t sDataDescriptionHashLength{ 4 };
  constexpr static std::array<DataSourceType, 0> sConsumedDataSources{};
  constexpr static std::array<DataSourceType, 0> sPublishedDataSources{};
  constexpr static std::array<ServiceRequest, 0> sRequiredServices{};
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::None };
  constexpr static bool sDetectorSpecific{ false };
  constexpr static Criticality sCriticality{ Criticality::Expendable };
};

// Minimal concrete actor satisfying mandatory interface
class DummyActor : public Actor<DummyActor>
{
 public:
  explicit DummyActor(const ServicesConfig& cfg) : Actor<DummyActor>(cfg) {}

  void onInit(framework::InitContext&) {}
  void onProcess(framework::ProcessingContext&) {}
};

TEST_CASE("A minimal dummy actor")
{
  // Traits must satisfy the concept enforced by Actor
  STATIC_CHECK(ValidActorTraits<ActorTraits<DummyActor>>);

  // Basic construction should be possible and not throw
  ServicesConfig cfg; // default activity and URLs are fine for construction (no services started)
  REQUIRE_NOTHROW(DummyActor{ cfg });
}

struct UnrequestedAccessActor;

template <>
struct ActorTraits<UnrequestedAccessActor> {
  constexpr static std::string_view sActorTypeShort{ "unrequested" };
  constexpr static std::string_view sActorTypeKebabCase{ "greedy-actor" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "GreedyActor" };
  constexpr static size_t sDataDescriptionHashLength{ 4 };
  constexpr static std::array<DataSourceType, 0> sConsumedDataSources{};
  constexpr static std::array<DataSourceType, 0> sPublishedDataSources{};
  constexpr static std::array<ServiceRequest, 0> sRequiredServices{};
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::None };
  constexpr static bool sDetectorSpecific{ false };
  constexpr static Criticality sCriticality{ Criticality::Expendable };
};

class UnrequestedAccessActor : public Actor<UnrequestedAccessActor>
{
 public:
  explicit UnrequestedAccessActor(const ServicesConfig& cfg) : Actor<UnrequestedAccessActor>(cfg) {}

  template <typename T>
  constexpr void assertNoAccessToServices()
  {
    static_assert(!(requires(T& t) { t.getMonitoring(); }));
    static_assert(!(requires(T& t) { t.getBookkeeping(); }));
    static_assert(!(requires(T& t) { t.getRepository(); }));
    static_assert(!(requires(T& t) { t.getCCDB(); }));
  }

  void onInit(framework::InitContext&)
  {
    assertNoAccessToServices<UnrequestedAccessActor>();
  }

  void onProcess(framework::ProcessingContext&) {}
};

TEST_CASE("An actor which tries to access services which it did not request")
{
  // Traits must satisfy the concept enforced by Actor
  STATIC_CHECK(ValidActorTraits<ActorTraits<UnrequestedAccessActor>>);

  // Basic construction should be possible and not throw
  ServicesConfig cfg; // default activity and URLs are fine for construction (no services started)
  REQUIRE_NOTHROW(UnrequestedAccessActor{ cfg });
}

} // namespace o2::quality_control::core
