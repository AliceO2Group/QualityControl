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

#ifndef QUALITYCONTROL_DATAPROCESSORADAPTER_H
#define QUALITYCONTROL_DATAPROCESSORADAPTER_H

///
/// \file   DataProcessorAdapter.h
/// \author Piotr Konopka
///

#include <Framework/DataProcessorSpec.h>

#include "QualityControl/Actor.h"
#include "QualityControl/ActorTraits.h"
#include "QualityControl/ActorHelpers.h"
#include "QualityControl/UserInputOutput.h"

namespace o2::quality_control::core
{

// helpers for DataProcessorAdapter
namespace impl
{

/// \brief checks if a type is derived a single UserCodeConfig
template <typename T>
concept UserCodeConfigSingle =
  std::derived_from<std::remove_cvref_t<T>, UserCodeConfig>;

/// \brief checks if a type is a range of UserCodeConfig children
template <typename R>
concept UserCodeConfigRange =
  std::ranges::input_range<R> &&
  std::derived_from<std::remove_cvref_t<std::ranges::range_value_t<R>>, UserCodeConfig>;

/// \brief converts scalars into ranges of length 1, preserves ranges
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

} // namespace impl

struct DataProcessorAdapter {

  /// \brief creates a DataProcessorSpec for a concrete actor
  template <typename ConcreteActor>
  static o2::framework::DataProcessorSpec
    adapt(ConcreteActor&& actor, std::string&& dataProcessorName, framework::Inputs&& inputs, framework::Outputs&& outputs, framework::Options&& options)
  {
    using traits = ActorTraits<ConcreteActor>;

    auto actorPtr = std::make_shared<ConcreteActor>(std::move(actor));
    o2::framework::DataProcessorSpec dataProcessor;

    dataProcessor.name = std::move(dataProcessorName);
    dataProcessor.inputs = std::move(inputs);
    dataProcessor.outputs = std::move(outputs);
    dataProcessor.options = std::move(options);

    dataProcessor.labels = { dataProcessorLabel<ConcreteActor>() };

    if constexpr (traits::sCriticality == Criticality::Resilient) {
      dataProcessor.labels.emplace_back("resilient");
    } else if constexpr (traits::sCriticality == Criticality::Critical) {
      // that's the default in DPL
    } else if constexpr (traits::sCriticality == Criticality::Expendable) {
      dataProcessor.labels.emplace_back("expendable");
    } else if constexpr (traits::sCriticality == Criticality::UserDefined) {
      if (!actor.isCritical()) {
        dataProcessor.labels.emplace_back("expendable");
      } else {
        // that's the default in DPL
      }
    }

    dataProcessor.algorithm = {
      [actorPtr](framework::InitContext& ictx) {
        actorPtr->init(ictx);
        return [actorPtr](framework::ProcessingContext& ctx) {
          actorPtr->process(ctx);
        };
      }
    };
    return dataProcessor;
  }

  /// \brief Produces a standard QC Data Processor name for cases when it runs user code and is associated with a detector.
  static std::string dataProcessorName(std::string_view userCodeName, std::string_view detectorName, std::string_view actorTypeKebabCase);

  /// \brief Produces a standard QC Data Processor name for cases when it runs user code and is associated with a detector.
  template <typename ConcreteActor>
    requires(actor_helpers::runsUserCode<ConcreteActor>() && ActorTraits<ConcreteActor>::sDetectorSpecific)
  static std::string dataProcessorName(std::string_view userCodeName, std::string_view detectorName)
  {
    using traits = ActorTraits<ConcreteActor>;
    return dataProcessorName(userCodeName, detectorName, traits::sActorTypeKebabCase);
  }

  /// \brief Produces standardized QC Data Processor name for cases were no user code is ran and it's not detector specific.
  template <typename ConcreteActor>
    requires(!actor_helpers::runsUserCode<ConcreteActor>() || !ActorTraits<ConcreteActor>::sDetectorSpecific)
  static std::string dataProcessorName()
  {
    using traits = ActorTraits<ConcreteActor>;
    return std::string{ traits::sActorTypeKebabCase };
  }

  /// \brief collects all user inputs in the provided UserCodeConfig(s) and returns framework::Inputs
  template <typename ConcreteActor, typename ConfigT>
    requires(impl::UserCodeConfigSingle<ConfigT> || impl::UserCodeConfigRange<ConfigT>)
  static framework::Inputs collectUserInputs(ConfigT&& config)
  {
    using traits = ActorTraits<ConcreteActor>;

    // normalize to a range, even if it's a single config
    auto configRange = impl::as_range(std::forward<ConfigT>(config));

    // get a view over all data sources
    auto dataSources = configRange //
                       | std::views::transform([](const UserCodeConfig& config) -> const auto& {
                           return config.dataSources;
                         }) |
                       std::views::join;

    // validate
    auto firstInvalid = std::ranges::find_if(dataSources, [](const DataSourceSpec& dataSource) {
      return std::ranges::none_of(traits::sConsumedDataSources, [&](const DataSourceType& allowed) {
        return dataSource.type == allowed;
      });
    });
    if (firstInvalid != dataSources.end()) {
      throw std::invalid_argument(
        std::format("DataSource '{}' is not one of supported types for '{}'", firstInvalid->id, traits::sActorTypeUpperCamelCase));
    }

    // copy into the results
    framework::Inputs inputs{};
    std::ranges::copy(dataSources                                                                        //
                        | std::views::transform([](const auto& ds) -> const auto& { return ds.inputs; }) //
                        | std::views::join,
                      std::back_inserter(inputs));

    // fixme: CheckRunner might have overlapping or repeating inputs. we should handle that here.
    //  There is some existing code in DataSampling which already does that, it could be copied here.

    return inputs;
  }

  /// \brief collects all user outputs in the provided UserCodeConfig(s) and returns framework::Outputs
  template <typename ConcreteActor, DataSourceType dataSourceType, typename ConfigT>
    requires(impl::UserCodeConfigSingle<ConfigT> || impl::UserCodeConfigRange<ConfigT>)
  static framework::Outputs collectUserOutputs(ConfigT&& config)
  {
    using traits = ActorTraits<ConcreteActor>;

    // normalize to a range, even if it's a single config
    auto configRange = impl::as_range(std::forward<ConfigT>(config));

    framework::Outputs outputs{};
    std::ranges::copy(configRange //
                        | std::views::transform([](const UserCodeConfig& config) {
                            return createUserOutputSpec(dataSourceType, config.detectorName, config.name);
                          }),
                      std::back_inserter(outputs));
    return outputs;
  }

  template <typename ConcreteActor>
  static framework::DataProcessorLabel dataProcessorLabel()
  {
    using traits = ActorTraits<ConcreteActor>;
    return { std::string{ traits::sActorTypeKebabCase } };
  }
};

}; // namespace o2::quality_control::core

#endif // QUALITYCONTROL_DATAPROCESSORADAPTER_H