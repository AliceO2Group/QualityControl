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
/// \file   Actor.h
/// \author Piotr Konopka
///

#ifndef ACTOR_H
#define ACTOR_H

#include <concepts>
#include <string_view>
#include <memory>
#include <type_traits>
#include <format>
#include <functional>

#include <Framework/CallbackService.h>
#include <Framework/InitContext.h>
#include <Framework/ProcessingContext.h>
#include <Framework/EndOfStreamContext.h>
#include <Framework/ConcreteDataMatcher.h>

#include "QualityControl/ActorTraits.h"
#include "QualityControl/ActorHelpers.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/ServicesConfig.h"

namespace o2::monitoring
{
class Monitoring;
}

namespace o2::bkp
{
enum class DplProcessType;
}

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::ccdb
{
class CCDBManagerInstance;
}

namespace o2::quality_control::core
{

class Bookkeeping;

// impl contains anything we want to hide in the source file to avoid exposing headers
namespace impl
{
std::shared_ptr<monitoring::Monitoring> initMonitoring(std::string_view url, std::string_view detector = "");
void startMonitoring(monitoring::Monitoring&, int runNumber);

void initBookkeeping(std::string_view url);
void startBookkeeping(int runNumber, std::string_view actorName, std::string_view detectorName, const o2::bkp::DplProcessType& processType, std::string_view args);
Bookkeeping& getBookkeeping();

std::shared_ptr<repository::DatabaseInterface> initRepository(const std::unordered_map<std::string, std::string>& config);

void initCCDB(const std::string& url);
ccdb::CCDBManagerInstance& getCCDB();

void handleExceptions(std::string_view when, const std::function<void()>&);
} // namespace impl

// Actor is a template base class for all QC Data Processors. It is supposed to bring their commonalities together,
// such as: service initialization, Data Processing Layer adoption, retrieving configuration and runtime parameters,
// interactions with controlling entities (DPL driver, AliECS, ODC).
//
// The design is based on CRTP (see the web for explanation), which allows us to:
// - avoid code repetition in implementing aforementioned commonalities
// - optionally perform certain actions depending on traits of an Actor specialization.
// CRTP, in contrast to dynamic inheritance, is also advertised to avoid performance impact due to vtable lookups.
// It is certainly a nice bonus in our case, but it was not the main motivation for CRTP-based approach.
//
// To allow for compile-time customization of centralized Actor features, we require each concrete Actor to implement
// an ActorTraits structure with certain parameters which is enforced with ValidActorTraits concept.
// The traits are separated from the main Actor class to improve readability and allow for shorter compilation times
// by allowing many helper functions avoid including Actor.h and a corresponding actor specialization. For additional
// savings on compilation time and clutter in code, we validate ActorTraits specializations with a concept only in
// Actor, but this could be revisited if proven wrong.
//
// To add a new QC Actor (please extend if something turns out to be missing):
// - define its ActorTraits
// - inherit Actor and implement it, e.g. class LateTaskRunner : public Actor<LateTaskRunner>
// - define a factory for the new Actor which uses DataProcessorAdapter to produce DataProcessorSpec
// - use the factory in relevant bits in InfrastructureGenerator
// - if the new Actor runs user code, one might need to add a *Spec structure and a corresponding reader in InfrastructureReader
// - add Actor-specific customizeInfrastructure to the rest in InfrastructureGenerator::customizeInfrastructure
//
// Next steps / ideas:
// - have a trait for CompletionPolicy, so that it is handled in one place, i.e. we don't have to add the same
//   boiler-plate for almost all actors.

template <typename ConcreteActor>
  requires ValidActorTraits<ActorTraits<ConcreteActor>>
class Actor
{
 private:
  // internal helpers
  using traits = ActorTraits<ConcreteActor>;

  static consteval bool runsUserCode() { return actor_helpers::runsUserCode<ConcreteActor>(); }

  template <ServiceRequest S>
  static consteval bool requiresService()
  {
    return actor_helpers::requiresService<ConcreteActor, S>();
  }

 public:
  explicit Actor(const ServicesConfig& servicesConfig)
    : mServicesConfig{ servicesConfig },
      mActivity{ servicesConfig.activity }
  {
    // compile-time (!) checks which can be performed only once ConcreteActor is a complete type, i.e. inside a function body
    // given that we declare mandatory methods as deleted, the compilation would still fail later.
    // this allows us to compile earlier and with clearer messages.
    assertCorrectConcreteActor();
  }

 public:
  void init(framework::InitContext& ictx)
  {
    impl::handleExceptions("process", [&] {
      // we set the fallback activity. fields might get overwritten once runtime values become available
      mActivity = mServicesConfig.activity;

      initServices(ictx);
      initDplCallbacks(ictx);

      concreteActor().onInit(ictx);
    });
  }

  void process(framework::ProcessingContext& ctx)
  {
    impl::handleExceptions("process", [&] {
      concreteActor().onProcess(ctx);
    });
  }

 protected:
  // mandatory methods to be implemented by concrete actor
  void onInit(framework::InitContext&) = delete;
  void onProcess(framework::ProcessingContext&) = delete;

  // mandatory methods to be implemented by concrete actor if specific features are enabled
  bool isCritical() const
    requires(traits::sCriticality == Criticality::UserDefined)
  = delete;
  std::string_view getDetectorName() const
    requires(traits::sDetectorSpecific)
  = delete;
  std::string_view getUserCodeName() const
    requires(runsUserCode())
  = delete;

  // optional methods that can be implemented by concrete actor
  void onStart(framework::ServiceRegistryRef services, const Activity& activity) {}
  void onStop(framework::ServiceRegistryRef services, const Activity& activity) {}
  void onReset(framework::ServiceRegistryRef services, const Activity& activity) {}
  void onEndOfStream(framework::EndOfStreamContext& eosContext) {}
  void onFinaliseCCDB(framework::ConcreteDataMatcher& matcher, void* obj) {}

  // service access for concrete actor
  std::reference_wrapper<monitoring::Monitoring> getMonitoring()
    requires(requiresService<ServiceRequest::Monitoring>())
  {
    return *mMonitoring;
  }
  std::reference_wrapper<Bookkeeping> getBookkeeping()
    requires(requiresService<ServiceRequest::Bookkeeping>())
  {
    return impl::getBookkeeping();
  }
  std::reference_wrapper<repository::DatabaseInterface> getRepository()
    requires(requiresService<ServiceRequest::QCDB>())
  {
    return *mRepository;
  }
  std::reference_wrapper<ccdb::CCDBManagerInstance> getCCDB()
    requires(requiresService<ServiceRequest::CCDB>())
  {
    return impl::getCCDB();
  }
  const Activity& getActivity() const
  {
    return mActivity;
  }

 private:
  static consteval void assertCorrectConcreteActor()
  {
    static_assert(std::derived_from<ConcreteActor, Actor<ConcreteActor>>);
    // mandatory methods
    static_assert(requires(ConcreteActor& actor, framework::ProcessingContext& pCtx) { { actor.onProcess(pCtx) } -> std::convertible_to<void>; });
    static_assert(requires(ConcreteActor& actor, framework::InitContext& iCtx) { { actor.onInit(iCtx) } -> std::convertible_to<void>; });

    // mandatory if specific features are enabled
    if constexpr (traits::sDetectorSpecific) {
      static_assert(requires(const ConcreteActor& actor) { { actor.getDetectorName() } -> std::convertible_to<std::string_view>; });
    }

    if constexpr (traits::sCriticality == Criticality::UserDefined) {
      static_assert(requires(const ConcreteActor& actor) { { actor.isCritical() } -> std::convertible_to<bool>; });
    }

    if constexpr (runsUserCode()) {
      static_assert(requires(const ConcreteActor& actor) { { actor.getUserCodeName() } -> std::convertible_to<std::string_view>; });
    }
  }

  // helpers to avoid repeated static_casts to call ConcreteActor methods
  ConcreteActor& concreteActor() { return static_cast<ConcreteActor&>(*this); }
  const ConcreteActor& concreteActor() const { return static_cast<const ConcreteActor&>(*this); }

  void initServices(framework::InitContext& ictx)
  {
    std::string detectorName;
    if constexpr (traits::sDetectorSpecific) {
      detectorName = std::string{ concreteActor().getDetectorName() };
    }

    if constexpr (requiresService<ServiceRequest::InfoLogger>()) {
      std::string facility;
      if constexpr (runsUserCode()) {
        facility = std::format("{}/{}", traits::sActorTypeShort, concreteActor().getUserCodeName());
      } else {
        facility = std::format("{}/", traits::sActorTypeShort);
      }

      // todo now we use the version from runnerUtils, but the implementation could be moved to Actor.cxx once we migrate all actors
      initInfologger(ictx, mServicesConfig.infologgerDiscardParameters, facility, detectorName);
    }
    if constexpr (requiresService<ServiceRequest::Monitoring>()) {
      mMonitoring = impl::initMonitoring(mServicesConfig.monitoringUrl, detectorName);
    }
    if constexpr (requiresService<ServiceRequest::Bookkeeping>()) {
      impl::initBookkeeping(mServicesConfig.bookkeepingUrl);
    }
    if constexpr (requiresService<ServiceRequest::QCDB>()) {
      mRepository = impl::initRepository(mServicesConfig.database);
    }
    if constexpr (requiresService<ServiceRequest::CCDB>()) {
      impl::initCCDB(mServicesConfig.conditionDBUrl);
    }
  }

  void initDplCallbacks(framework::InitContext& ictx)
  {
    try {
      auto& callbacks = ictx.services().get<framework::CallbackService>();

      // we steal services reference, because it is not available as an argument of these callbacks
      framework::ServiceRegistryRef services = ictx.services();

      callbacks.set<framework::CallbackService::Id::Start>([this, services]() { this->start(services); });
      callbacks.set<framework::CallbackService::Id::Stop>([this, services]() { this->stop(services); });
      callbacks.set<framework::CallbackService::Id::Reset>([this, services]() { this->reset(services); });
      callbacks.set<framework::CallbackService::Id::EndOfStream>(
        [this](framework::EndOfStreamContext& eosContext) { this->endOfStream(eosContext); });
      callbacks.set<framework::CallbackService::Id::CCDBDeserialised>(
        [this](framework::ConcreteDataMatcher& matcher, void* obj) { this->finaliseCCDB(matcher, obj); });
    } catch (framework::RuntimeErrorRef& ref) {
      ILOG(Fatal) << "Error during callback registration: " << framework::error_from_ref(ref).what << ENDM;
      throw;
    }
  }

  void start(framework::ServiceRegistryRef services)
  {
    impl::handleExceptions("start", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " start" << ENDM;

      mActivity = computeActivity(services, mActivity);

      if constexpr (requiresService<ServiceRequest::InfoLogger>()) {
        QcInfoLogger::setRun(mActivity.mId);
        QcInfoLogger::setPartition(mActivity.mPartitionName);
      }
      if constexpr (requiresService<ServiceRequest::Monitoring>()) {
        impl::startMonitoring(*mMonitoring, mActivity.mId);
      }
      if constexpr (requiresService<ServiceRequest::Bookkeeping>()) {
        std::string actorName;
        if constexpr (runsUserCode()) {
          actorName = concreteActor().getUserCodeName();
        } else {
          actorName = traits::sActorTypeKebabCase;
        }

        std::string detectorName;
        if constexpr (traits::sDetectorSpecific) {
          detectorName = concreteActor().getDetectorName();
        }

        // todo: get args
        impl::startBookkeeping(mActivity.mId, actorName, detectorName, traits::sDplProcessType, "");
      }

      concreteActor().onStart(services, mActivity);
    });
  }

  void stop(framework::ServiceRegistryRef services)
  {
    impl::handleExceptions("stop", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " stop" << ENDM;

      mActivity = computeActivity(services, mActivity);

      concreteActor().onStop(services, mActivity);
    });
  }

  void reset(framework::ServiceRegistryRef services)
  {
    impl::handleExceptions("reset", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " reset" << ENDM;

      mActivity = mServicesConfig.activity;

      concreteActor().onReset(services, mActivity);
    });
  }

  void endOfStream(framework::EndOfStreamContext& eosContext)
  {
    impl::handleExceptions("endOfStream", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " endOfStream" << ENDM;

      concreteActor().onEndOfStream(eosContext);
    });
  }
  void finaliseCCDB(framework::ConcreteDataMatcher& matcher, void* obj)
  {
    impl::handleExceptions("finaliseCCDB", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " finaliseCCDB" << ENDM;

      concreteActor().onFinaliseCCDB(matcher, obj);
    });
  }

 private:
  Activity mActivity;
  const ServicesConfig mServicesConfig;

  std::shared_ptr<monitoring::Monitoring> mMonitoring;
  std::shared_ptr<repository::DatabaseInterface> mRepository;
};

} // namespace o2::quality_control::core
#endif // ACTOR_H