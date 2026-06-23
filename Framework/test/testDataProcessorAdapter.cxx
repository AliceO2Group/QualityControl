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
/// \file    testDataProcessorAdapter.cxx
/// \author  Piotr Konopka
///

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

#include <catch_amalgamated.hpp>

#include <Framework/DataProcessorLabel.h>
#include <Framework/DataSpecUtils.h>

#include "QualityControl/DataProcessorAdapter.h"
#include "QualityControl/UserCodeConfig.h"
#include "QualityControl/UserInputOutput.h"

namespace o2::quality_control::core
{

using o2::framework::DataProcessorLabel;
using o2::framework::DataSpecUtils;
using o2::framework::Inputs;
using o2::framework::Options;
using o2::framework::Outputs;

bool hasLabel(const std::vector<DataProcessorLabel>& labels, std::string_view value)
{
  return std::find(labels.begin(), labels.end(), DataProcessorLabel{ std::string{ value } }) != labels.end();
}

struct ResilientActor {
  void init(o2::framework::InitContext&) {}
  void process(o2::framework::ProcessingContext&) {}
};

template <>
struct ActorTraits<ResilientActor> {
  constexpr static std::string_view sActorTypeKebabCase{ "qc-criticality-resilient-actor" };
  constexpr static Criticality sCriticality{ Criticality::Resilient };
};

TEST_CASE("DataProcessorAdapter::adapt adds resilient label for resilient criticality")
{
  auto spec = DataProcessorAdapter::adapt(ResilientActor{}, "resilient-dp", Inputs{}, Outputs{}, Options{});
  CHECK(hasLabel(spec.labels, "resilient"));
  CHECK_FALSE(hasLabel(spec.labels, "expendable"));
}

struct CriticalActor {
  void init(o2::framework::InitContext&) {}
  void process(o2::framework::ProcessingContext&) {}
};

template <>
struct ActorTraits<CriticalActor> {
  constexpr static std::string_view sActorTypeKebabCase{ "qc-criticality-critical-actor" };
  constexpr static Criticality sCriticality{ Criticality::Critical };
};

TEST_CASE("DataProcessorAdapter::adapt keeps default labels for critical criticality")
{
  auto spec = DataProcessorAdapter::adapt(CriticalActor{}, "critical-dp", Inputs{}, Outputs{}, Options{});
  CHECK_FALSE(hasLabel(spec.labels, "resilient"));
  CHECK_FALSE(hasLabel(spec.labels, "expendable"));
}

struct ExpendableActor {
  void init(o2::framework::InitContext&) {}
  void process(o2::framework::ProcessingContext&) {}
};

template <>
struct ActorTraits<ExpendableActor> {
  constexpr static std::string_view sActorTypeKebabCase{ "qc-criticality-expendable-actor" };
  constexpr static Criticality sCriticality{ Criticality::Expendable };
};

TEST_CASE("DataProcessorAdapter::adapt adds expendable label for expendable criticality")
{
  auto spec = DataProcessorAdapter::adapt(ExpendableActor{}, "expendable-dp", Inputs{}, Outputs{}, Options{});
  CHECK(hasLabel(spec.labels, "expendable"));
  CHECK_FALSE(hasLabel(spec.labels, "resilient"));
}

struct UserDefinedCriticalityActor {
  explicit UserDefinedCriticalityActor(bool isCritical)
    : mIsCritical{ isCritical }
  {
  }

  bool isCritical() const { return mIsCritical; }
  void init(o2::framework::InitContext&) {}
  void process(o2::framework::ProcessingContext&) {}

 private:
  bool mIsCritical;
};

template <>
struct ActorTraits<UserDefinedCriticalityActor> {
  constexpr static std::string_view sActorTypeKebabCase{ "qc-userdefined-criticality-actor" };
  constexpr static Criticality sCriticality{ Criticality::UserDefined };
};

TEST_CASE("DataProcessorAdapter::adapt uses actor critical flag for user-defined criticality")
{
  SECTION("critical actor instance")
  {
    auto spec = DataProcessorAdapter::adapt(UserDefinedCriticalityActor{ true }, "userdefined-critical", Inputs{}, Outputs{}, Options{});
    // that's not a mistake, "resilient" means the task itself critical, but can survive crashes of upstream data processors.
    // this way we allow for upstream data processors to be either critical or expendable and hide this complexity from the user.
    CHECK(hasLabel(spec.labels, "resilient"));
  }

  SECTION("non-critical actor instance")
  {
    auto spec = DataProcessorAdapter::adapt(UserDefinedCriticalityActor{ false }, "userdefined-expendable", Inputs{}, Outputs{}, Options{});
    CHECK(hasLabel(spec.labels, "expendable"));
  }
}

struct ActorAlice;

template <>
struct ActorTraits<ActorAlice> {
  constexpr static std::string_view sActorTypeKebabCase{ "qc-actor-a" };
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::None };
  constexpr static bool sDetectorSpecific{ false };
};

struct ActorBob {
  void init(o2::framework::InitContext&) {}
  void process(o2::framework::ProcessingContext&) {}
};

template <>
struct ActorTraits<ActorBob> {
  constexpr static std::string_view sActorTypeKebabCase{ "qc-actor-b" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "QcActorBob" };
  constexpr static std::array<DataSourceType, 2> sConsumedDataSources{ DataSourceType::Task, DataSourceType::Check };
  constexpr static Criticality sCriticality{ Criticality::Critical };
};

TEST_CASE("DataProcessorAdapter::dataProcessorName validates detector")
{
  CHECK(DataProcessorAdapter::dataProcessorName("taskName", "TPC", "qc-task") == "qc-task-TPC-taskName");
  CHECK(DataProcessorAdapter::dataProcessorName("taskName", "INVALID", "qc-task") == "qc-task-MISC-taskName");
}

TEST_CASE("DataProcessorAdapter::dataProcessorName without user code")
{
  CHECK(DataProcessorAdapter::dataProcessorName<ActorAlice>() == "qc-actor-a");
}

TEST_CASE("DataProcessorAdapter::adapt forwards processor specs")
{
  const Inputs inputs{ createUserInputSpec(DataSourceType::Task, "TPC", "taskInput") };
  const Outputs outputs{ createUserOutputSpec(DataSourceType::Task, "TPC", "taskOutput") };

  auto spec = DataProcessorAdapter::adapt(ActorBob{}, "io-dp", Inputs{ inputs }, Outputs{ outputs }, Options{});

  CHECK(spec.name == "io-dp");
  REQUIRE(spec.inputs.size() == 1);
  REQUIRE(spec.outputs.size() == 1);
  CHECK(DataSpecUtils::match(spec.inputs[0], DataSpecUtils::asConcreteDataMatcher(inputs[0])));
  CHECK(DataSpecUtils::match(spec.outputs[0], DataSpecUtils::asConcreteDataMatcher(outputs[0])));
}

TEST_CASE("DataProcessorAdapter::collectUserInputs handles single config and ranges")
{
  UserCodeConfig configA;
  configA.name = "taskA";
  configA.detectorName = "TPC";
  configA.dataSources = {
    DataSourceSpec{ DataSourceType::Task },
    DataSourceSpec{ DataSourceType::Check }
  };
  configA.dataSources[0].id = "task-source";
  configA.dataSources[0].inputs = { createUserInputSpec(DataSourceType::Task, "TPC", "taskA") };
  configA.dataSources[1].id = "check-source";
  configA.dataSources[1].inputs = {
    createUserInputSpec(DataSourceType::Check, "TPC", "checkA", 0, "checkA_binding"),
    createUserInputSpec(DataSourceType::Check, "TPC", "checkA", 1, "checkA_binding_1")
  };

  SECTION("single config")
  {
    const auto inputs = DataProcessorAdapter::collectUserInputs<ActorBob>(configA);

    REQUIRE(inputs.size() == 3);
    CHECK(DataSpecUtils::match(inputs[0], DataSpecUtils::asConcreteDataMatcher(configA.dataSources[0].inputs[0])));
    CHECK(DataSpecUtils::match(inputs[1], DataSpecUtils::asConcreteDataMatcher(configA.dataSources[1].inputs[0])));
    CHECK(DataSpecUtils::match(inputs[2], DataSpecUtils::asConcreteDataMatcher(configA.dataSources[1].inputs[1])));
  }

  SECTION("range of configs")
  {
    UserCodeConfig configB;
    configB.name = "taskB";
    configB.detectorName = "TPC";
    configB.dataSources = { DataSourceSpec{ DataSourceType::Task } };
    configB.dataSources[0].id = "taskB-source";
    configB.dataSources[0].inputs = {
      createUserInputSpec(DataSourceType::Task, "TPC", "taskB", 2, "taskB_binding")
    };

    std::vector<UserCodeConfig> configs{ configA, configB };
    const auto inputs = DataProcessorAdapter::collectUserInputs<ActorBob>(configs);

    REQUIRE(inputs.size() == 4);
    CHECK(DataSpecUtils::match(inputs[0], DataSpecUtils::asConcreteDataMatcher(configA.dataSources[0].inputs[0])));
    CHECK(DataSpecUtils::match(inputs[1], DataSpecUtils::asConcreteDataMatcher(configA.dataSources[1].inputs[0])));
    CHECK(DataSpecUtils::match(inputs[2], DataSpecUtils::asConcreteDataMatcher(configA.dataSources[1].inputs[1])));
    CHECK(DataSpecUtils::match(inputs[3], DataSpecUtils::asConcreteDataMatcher(configB.dataSources[0].inputs[0])));
  }
}

TEST_CASE("DataProcessorAdapter::collectUserInputs rejects unsupported source type")
{
  UserCodeConfig config;
  config.dataSources = { DataSourceSpec{ DataSourceType::Direct } };
  config.dataSources[0].id = "unsupported-source";

  REQUIRE_THROWS_AS((DataProcessorAdapter::collectUserInputs<ActorBob>(config)), std::invalid_argument);
}

TEST_CASE("DataProcessorAdapter::collectUserOutputs handles single config and ranges")
{
  UserCodeConfig configA;
  configA.name = "taskA";
  configA.detectorName = "TPC";

  UserCodeConfig configB;
  configB.name = "taskB";
  configB.detectorName = "TRD";

  SECTION("single config")
  {
    const auto outputs = DataProcessorAdapter::collectUserOutputs<ActorBob, DataSourceType::Task>(configA);

    REQUIRE(outputs.size() == 1);
    CHECK(outputs[0].binding.value == "taskA");
    CHECK(DataSpecUtils::match(outputs[0], DataSpecUtils::asConcreteDataMatcher(createUserOutputSpec(DataSourceType::Task, "TPC", "taskA"))));
  }

  SECTION("range of configs")
  {
    std::vector<UserCodeConfig> configs{ configA, configB };
    const auto outputs = DataProcessorAdapter::collectUserOutputs<ActorBob, DataSourceType::Aggregator>(configs);

    REQUIRE(outputs.size() == 2);
    CHECK(outputs[0].binding.value == "taskA");
    CHECK(outputs[1].binding.value == "taskB");
    CHECK(DataSpecUtils::match(outputs[0], DataSpecUtils::asConcreteDataMatcher(createUserOutputSpec(DataSourceType::Aggregator, "TPC", "taskA"))));
    CHECK(DataSpecUtils::match(outputs[1], DataSpecUtils::asConcreteDataMatcher(createUserOutputSpec(DataSourceType::Aggregator, "TRD", "taskB"))));
  }
}

} // namespace o2::quality_control::core
