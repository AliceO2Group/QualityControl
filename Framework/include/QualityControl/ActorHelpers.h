//
// Created by pkonopka on 07/11/25.
//

#ifndef ACTORFACTORY_H
#define ACTORFACTORY_H

#include "QualityControl/InfrastructureSpecReader.h"
#include "QualityControl/ActorTraits.h"
#include "QualityControl/InputUtils.h"
#include "QualityControl/ServicesConfig.h"
#include "QualityControl/UserCodeConfig.h"
#include <format>
#include <ranges>
#include <Framework/DataProcessorSpec.h>

namespace o2::quality_control::core {

namespace impl {

// todo move somewhere?
template <typename T>
concept UserCodeConfigSingle =
  std::derived_from<std::remove_cvref_t<T>, UserCodeConfig>;

template <typename R>
concept UserCodeConfigRange =
  std::ranges::input_range<R> &&
  std::derived_from<std::remove_cvref_t<std::ranges::range_value_t<R>>, UserCodeConfig>;

// todo move somewhere?
template <class T>
auto as_range(T&& t)
{
  using U = std::remove_reference_t<T>;

  if constexpr (std::ranges::range<U>) {
    // Already a range, just wrap in a view for consistency
    return std::views::all(std::forward<T>(t));
  } else {
    // a scalar, we wrap it into a single-element range
    return std::views::single(std::forward<T>(t));
  }
}

}

struct CommonSpec;

namespace actor_helpers
{
  ServicesConfig extractConfig(const CommonSpec& commonSpec);

  template<typename ConcreteActor>
  requires (ValidActorTraits<ActorTraits<ConcreteActor>> &&
            runsUserCode<ActorTraits<ConcreteActor>>() &&
            ActorTraits<ConcreteActor>::sDetectorSpecific)
  std::string dataProcessorName(std::string_view userCodeName, std::string_view detectorName)
  {
    using traits = ActorTraits<ConcreteActor>;
    // todo move implementation to src (will avoid exposing <format>, InfrastructureSpecReader.h)
    // todo perhaps detector name validation should happen earlier, just once and throw in case of configuration errors
    return std::format("{}-{}-{}", traits::sActorTypeKebabCase, InfrastructureSpecReader::validateDetectorName(std::string{detectorName}), userCodeName);
  }

  template<typename ConcreteActor>
  requires (ValidActorTraits<ActorTraits<ConcreteActor>>)
  std::string dataProcessorName()
  {
    using traits = ActorTraits<ConcreteActor>;
    return std::string{traits::sActorTypeKebabCase};
  }

  template <typename ConcreteActor, typename ConfigT>
  requires (ValidActorTraits<ActorTraits<ConcreteActor>> &&
           (impl::UserCodeConfigSingle<ConfigT> || impl::UserCodeConfigRange<ConfigT>))
  framework::Inputs collectUserInputs(ConfigT&& config)
  {
    using traits = ActorTraits<ConcreteActor>;

    // normalize to a range, even if it's a single config
    auto configRange = impl::as_range(std::forward<ConfigT>(config));

    // get a view over all data sources
    auto dataSources = configRange
      | std::views::transform([](const UserCodeConfig& config) -> const auto& {
          return config.dataSources;
        })
      | std::views::join;

    // validate
    auto firstInvalid = std::ranges::find_if(dataSources, [](const DataSourceSpec& dataSource) {
          return std::ranges::none_of(traits::sConsumedDataSources, [&](const DataSourceType& allowed) {
            return dataSource.type == allowed;
          });
      });

    if (firstInvalid != dataSources.end()) {
      throw std::invalid_argument(
        std::format("DataSource '{}' is not one of supported types for '{}'", firstInvalid->id, traits::sActorTypeUpperCamelCase)
      );
    }

    // copy into the results
    framework::Inputs inputs{};
    std::ranges::copy(dataSources
      | std::views::transform([](const auto& ds) -> const auto& { return ds.inputs; })
      | std::views::join,
      std::back_inserter(inputs));

    // fixme: CheckRunner might have overlapping or repeating inputs. we should handle that here.
    //  There is some existing code in DataSampling which already does that, it could be copied here.
    return inputs;
  }

  template <typename ConcreteActor, DataSourceType dataSourceType, typename ConfigT>
  requires (ValidActorTraits<ActorTraits<ConcreteActor>> &&
           (impl::UserCodeConfigSingle<ConfigT> || impl::UserCodeConfigRange<ConfigT>))
  framework::Outputs collectUserOutputs(ConfigT&& config)
  {
    using traits = ActorTraits<ConcreteActor>;

    // normalize to a range, even if it's a single config
    auto configRange = impl::as_range(std::forward<ConfigT>(config));

    framework::Outputs outputs{};
    std::ranges::copy(configRange
      | std::views::transform([](const UserCodeConfig& config) {
        return createUserOutputSpec<ConcreteActor, dataSourceType>(config.detectorName, config.name);
      }),
      std::back_inserter(outputs)
    );
    return outputs;
  }

  template<typename ConcreteActor>
  requires ValidActorTraits<ActorTraits<ConcreteActor>>
  framework::DataProcessorLabel dataProcessorLabel()
  {
    using traits = ActorTraits<ConcreteActor>;
    return { std::string{traits::sActorTypeKebabCase} };
  }

}

}


#endif //ACTORFACTORY_H
