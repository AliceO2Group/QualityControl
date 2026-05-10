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
/// \file    testActorCallbacks.cxx
/// \author  Piotr Konopka
///

#include "QualityControl/Actor.h"
#include "QualityControl/ActorTraits.h"
#include "QualityControl/ActorHelpers.h"
#include "QualityControl/DataProcessorAdapter.h"

#include <Framework/runDataProcessing.h>
#include <Framework/ControlService.h>

using namespace o2::framework;
using namespace o2::quality_control::core;

struct DummyActor;

template <>
struct o2::quality_control::core::ActorTraits<DummyActor> {
  constexpr static std::string_view sActorTypeShort{ "dummy" };
  constexpr static std::string_view sActorTypeKebabCase{ "qc-dummy-actor" };
  constexpr static std::string_view sActorTypeUpperCamelCase{ "DummyActor" };
  constexpr static size_t sDataDescriptionHashLength{ 4 };
  constexpr static std::array<DataSourceType, 0> sConsumedDataSources{};
  constexpr static std::array<DataSourceType, 0> sPublishedDataSources{};
  constexpr static std::array<ServiceRequest, 0> sRequiredServices{};
  constexpr static UserCodeCardinality sUserCodeCardinality{ UserCodeCardinality::None };
  constexpr static bool sDetectorSpecific{ false };
  constexpr static Criticality sCriticality{ Criticality::Critical };
};

// test helpers
constexpr std::string_view sEventCreated = "Created";
constexpr std::string_view sEventOnInitCalled = "onInit called";
constexpr std::string_view sEventOnStartCalled = "onStart called";
constexpr std::string_view sEventOnProcessCalled = "onProcess called";
constexpr std::string_view sEventOnStopCalled = "onStop called";
constexpr std::string_view sEventOnResetCalled = "onReset called";

// Minimal concrete actor satisfying mandatory interface
class DummyActor : public Actor<DummyActor>
{
 public:
  explicit DummyActor(const ServicesConfig& cfg) : Actor<DummyActor>(cfg) {}
  ~DummyActor() = default;

  void onInit(InitContext&)
  {
    LOG(info) << "onInit called";
    if (mLastEvent != sEventCreated) {
      LOG(fatal) << "test failed in onInit, last event should have been '" << sEventCreated << "', but was '" << mLastEvent << "'";
    }
    mLastEvent = sEventOnInitCalled;
  }

  void onStart(ServiceRegistryRef services, const Activity& activity)
  {
    LOG(info) << "onStart called";
    if (mLastEvent != sEventOnInitCalled) {
      LOG(fatal) << "test failed in onStart, last event should have been '" << sEventOnInitCalled << "', but was '" << mLastEvent << "'";
    }
    mLastEvent = sEventOnStartCalled;
  }

  void onProcess(ProcessingContext& ctx)
  {
    LOG(info) << "onProcess called";
    if (mLastEvent != sEventOnStartCalled) {
      LOG(fatal) << "test failed in onProcess, last event should have been '" << sEventOnStartCalled << "', but was '" << mLastEvent << "'";
    }
    mLastEvent = sEventOnProcessCalled;
    ctx.services().get<ControlService>().endOfStream();
  }

  void onStop(ServiceRegistryRef services, const Activity& activity)
  {
    LOG(info) << "onStop called";
    if (mLastEvent != sEventOnProcessCalled) {
      LOG(fatal) << "test failed in onStop, last event should have been '" << sEventOnProcessCalled << "', but was '" << mLastEvent << "'";
    }
    mLastEvent = sEventOnStopCalled;
  }

  void onReset(ServiceRegistryRef services, const Activity& activity)
  {
    LOG(info) << "onReset called";
    if (mLastEvent != sEventOnStopCalled) {
      LOG(fatal) << "test failed in onReset, last event should have been '" << sEventOnStopCalled << "', but was '" << mLastEvent << "'";
    }
    mLastEvent = sEventOnResetCalled;
  }

 private:
  std::string_view mLastEvent = sEventCreated;
};

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  WorkflowSpec specs;

  ServicesConfig cfg;
  DummyActor dummyActor{ cfg };

  specs.push_back(DataProcessorAdapter::adapt(
    std::move(dummyActor),
    "dummy-actor",
    Inputs{},
    Outputs{ { { "out" }, "TST", "DUMMY", 0 } },
    Options{}));

  // if dummy actor never sends EoS, this receiver idles until timeout and the test fails.
  specs.push_back({ "receiver",
                    Inputs{ { { "in" }, "TST", "DUMMY", 0 } },
                    Outputs{} });

  return specs;
}