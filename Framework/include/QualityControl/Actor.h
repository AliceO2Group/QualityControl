//
// Created by pkonopka on 23/06/25.
//

#ifndef ACTOR_H
#define ACTOR_H

#include <concepts>
#include <string_view>
#include <memory>
#include <type_traits>
#include <format>
#include <functional>

#include "QualityControl/ActorTraits.h"
#include "QualityControl/runnerUtils.h"
#include "QualityControl/ServicesConfig.h"

#include <Framework/CallbackService.h>
#include <Framework/InitContext.h>
#include <Framework/ProcessingContext.h>
#include <Framework/EndOfStreamContext.h>
#include <Framework/ConcreteDataMatcher.h>


namespace o2::monitoring {
class Monitoring;
}

namespace o2::bkp {
enum class DplProcessType;
}


namespace o2::quality_control::repository {
class DatabaseInterface;
}

namespace o2::ccdb {
class CCDBManagerInstance;
}

namespace o2::quality_control::core {


class Bookkeeping;

// anything we want to hide in the source file to avoid exposing headers
namespace impl
{
// fixme: to my understanding, we cannot declare these are Actor members and have them compiled within .cxx, because it's a template
std::shared_ptr<monitoring::Monitoring> initMonitoring(std::string_view url, std::string_view detector = "");
void startMonitoring(monitoring::Monitoring&, int runNumber);

void initBookkeeping(std::string_view url);
void startBookkeeping(int runNumber, std::string_view actorName, std::string_view detectorName, const o2::bkp::DplProcessType& processType, std::string_view args);
Bookkeeping& getBookkeeping();

std::shared_ptr<repository::DatabaseInterface> initRepository(const std::unordered_map<std::string, std::string>& config);

void initCCDB(const std::string& url);
ccdb::CCDBManagerInstance& getCCDB();

void handleExceptions(std::string_view when, const std::function<void()>&);

}


// todo consider having a check to avoid overriding Actor methods
// Actor is supposed to contain any common logic for QC actors (starting services, handling exceptions) and bridging with DPL (specs, registering callbacks).
template<typename ConcreteActor>
requires ValidActorTraits<ActorTraits<ConcreteActor>>
class Actor
{
  private:
  using traits = ActorTraits<ConcreteActor>;

  // helpers to avoid repeated static_casts to call ConcreteActor methods
  ConcreteActor& concreteActor() { return static_cast<ConcreteActor&>(*this); }
  const ConcreteActor& concreteActor() const { return static_cast<const ConcreteActor&>(*this); }

  // a trick to prevent bugs like "class Derived2 : public Base<Derived1>"
  // see https://www.fluentcpp.com/2017/05/12/curiously-recurring-template-pattern/
  Actor(){};
  friend ConcreteActor;

  // helper to check if ConcreteActor needs a given service
  template<Service S>
  consteval bool requiresService() {
    for (const auto& required : traits::sRequiredServices) {
      if (required == S) {
        return true;
      }
    }
    return false;
    // todo: when we can use C++23
    // return std::ranges::contains(ActorTraitsT::sRequiredServices, S);
  }

  public:


  explicit Actor(const ServicesConfig& servicesConfig) :
    mServicesConfig{servicesConfig},
    mActivity(servicesConfig.activity)
  {
    // compile-time (!) checks which can be performed only once ConcreteActor is a complete type, i.e. inside a function body
    assertCorrectConcreteActor();

  }

  consteval void assertCorrectConcreteActor() const
  {
    // mandatory methods
    static_assert( requires(ConcreteActor& actor, framework::ProcessingContext& pCtx) { { actor.onProcess(pCtx) } -> std::convertible_to<void>; });
    static_assert( requires(ConcreteActor& actor, framework::InitContext& iCtx) { { actor.onInit(iCtx) } -> std::convertible_to<void>; });

    // mandatory if specific features are enabled
    if constexpr (traits::sDetectorSpecific) {
      static_assert( requires(const ConcreteActor& actor) { { actor.getDetectorName() } -> std::convertible_to<std::string_view>; });
    }

    if constexpr(traits::sCriticality == Criticality::UserDefined) {
      static_assert( requires(const ConcreteActor& actor) { { actor.isCritical() } -> std::convertible_to<bool>; });
    }

    if constexpr (runsUserCode<traits>()) {
      static_assert(requires(const ConcreteActor& actor) { { actor.getUserCodeName() } -> std::convertible_to<std::string_view>; });
    }
  }

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

  void initServices(framework::InitContext& ictx)
  {
    std::string detectorName;
    if constexpr (traits::sDetectorSpecific) {
      detectorName = std::string{concreteActor().getDetectorName()};
    }

    if constexpr (requiresService<Service::InfoLogger>()) {
      std::string facility;
      if constexpr (runsUserCode<traits>()) {
        facility = std::format("{}/{}", traits::sActorTypeShort, concreteActor().getUserCodeName());
      } else {
        facility = std::format("{}/", traits::sActorTypeShort);
      }

      // todo now we use the version from runnerUtils, but the implementation could be moved to Actor.cxx once we migrate all actors
      initInfologger(ictx, mServicesConfig.infologgerDiscardParameters, facility, detectorName);
    }
    if constexpr (requiresService<Service::Monitoring>()) {
      mMonitoring = impl::initMonitoring(mServicesConfig.monitoringUrl, detectorName);
    }
    if constexpr (requiresService<Service::Bookkeeping>()) {
      impl::initBookkeeping(mServicesConfig.bookkeepingUrl);
    }
    if constexpr (requiresService<Service::QCDB>()) {
      mRepository = impl::initRepository(mServicesConfig.database);
    }
    if constexpr (requiresService<Service::CCDB>()) {
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

      // we register optional callbacks like that:
      if constexpr (requires { &ConcreteActor::endOfStream; }) {
        callbacks.set<framework::CallbackService::Id::EndOfStream>([this](framework::EndOfStreamContext& eosContext) {
          this->endOfStream(eosContext);
        });
      }
      if constexpr (requires { &ConcreteActor::finaliseCCDB; }) {
        callbacks.set<framework::CallbackService::Id::CCDBDeserialised>([this](framework::ConcreteDataMatcher& matcher, void* obj) {
          this->finaliseCCDB(matcher, obj);
        });
      }
    } catch (framework::RuntimeErrorRef& ref) {
      ILOG(Fatal) << "Error during callback registration: " << framework::error_from_ref(ref).what << ENDM;
      throw;
    }
  }


  void process(framework::ProcessingContext& ctx)
  {
    impl::handleExceptions("process", [&] {
      concreteActor().onProcess(ctx);
    });
  }

  void start(framework::ServiceRegistryRef services) {
    impl::handleExceptions("start", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " start" << ENDM;

      mActivity = computeActivity(services, mActivity);

      if constexpr (requiresService<Service::InfoLogger>()) {
        QcInfoLogger::setRun(mActivity.mId);
        QcInfoLogger::setPartition(mActivity.mPartitionName);
      }
      if constexpr (requiresService<Service::Monitoring>()) {
        impl::startMonitoring(*mMonitoring, mActivity.mId);
      }
      if constexpr (requiresService<Service::Bookkeeping>()) {
        std::string actorName;
        if constexpr (runsUserCode<traits>()) {
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

      // fixme: should we just have empty methods in the base class for the sake of readability?
      if constexpr (requires {ConcreteActor::onStart;}) {
        concreteActor().onStart(services, mActivity);
      }
    });
  }

  void stop(framework::ServiceRegistryRef services) {
    impl::handleExceptions("stop", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " stop" << ENDM;

      mActivity = computeActivity(services, mActivity);

      if constexpr (requires {ConcreteActor::onStop;}) {
        concreteActor().onStop(services, mActivity);
      }
    });
  }

  void reset(framework::ServiceRegistryRef services) {
    impl::handleExceptions("reset", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " reset" << ENDM;

      mActivity = mServicesConfig.activity;

      if constexpr (requires {ConcreteActor::onReset;}) {
        concreteActor().onReset(services, mActivity);
      }
    });
  }

  void endOfStream(framework::EndOfStreamContext& eosContext) {
    impl::handleExceptions("endOfStream", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " endOfStream" << ENDM;
      if constexpr (requires {ConcreteActor::onEndOfStream;}) {
        concreteActor().onEndOfStream(eosContext);
      }
    });
  }
  void finaliseCCDB(framework::ConcreteDataMatcher& matcher, void* obj)
  {
    impl::handleExceptions("finaliseCCDB", [&] {
      ILOG(Debug, Trace) << traits::sActorTypeKebabCase << " finaliseCCDB" << ENDM;
      if constexpr (requires {ConcreteActor::onFinaliseCCDB;}) {
        concreteActor().onFinaliseCCDB(matcher, obj);
      }
    });
  }

  protected:

  monitoring::Monitoring& getMonitoring() requires (requiresService<Service::Monitoring>())
  {
    return *mMonitoring;
  }

  Bookkeeping& getBookkeeping() requires (requiresService<Service::Bookkeeping>())
  {
    return impl::getBookkeeping();
  }

  repository::DatabaseInterface& getRepository() requires (requiresService<Service::QCDB>())
  {
    return *mRepository;
  }

  ccdb::CCDBManagerInstance& getCCDB() requires (requiresService<Service::QCDB>())
  {
    return impl::getCCDB();
  }

  const Activity& getActivity() const { return mActivity; }

 private:
  Activity mActivity;
  const ServicesConfig mServicesConfig;

  std::shared_ptr<monitoring::Monitoring> mMonitoring;
  std::shared_ptr<repository::DatabaseInterface> mRepository;
};


}
#endif //ACTOR_H
