// Copyright 2025 CERN and copyright holders of ALICE O2.
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
/// \file   Data.h
/// \author Michal Tichak
///

#ifndef QC_CORE_DATA_H
#define QC_CORE_DATA_H

#include <any>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <ranges>
#include <type_traits>

namespace o2::quality_control::core
{

template <typename Function, typename Result, typename... Args>
concept invocable_r = std::invocable<Function, Args...> && std::same_as<std::invoke_result_t<Function, Args...>, Result>;

class Data
{
 public:
  Data() = default;

  template <typename Result>
  std::optional<Result> get(std::string_view key)
  {
    if (const auto foundIt = mObjects.find(key); foundIt != mObjects.end()) {
      if (auto* casted = std::any_cast<Result>(&foundIt->second); casted != nullptr) {
        return { *casted };
      }
    }
    return std::nullopt;
  }

  template <typename T, typename... Args>
  void emplace(std::string_view key, Args&&... args)
  {
    mObjects.emplace(key, std::any{ std::in_place_type<T>, std::forward<Args>(args)... });
  }

  template <typename T>
  void insert(std::string_view key, const T& value)
  {
    mObjects.insert({ std::string{ key }, value });
  }

  template <typename T>
  static const T* any_cast_try_shared_ptr(const std::any& value)
  {
    // sadly it is necessary to check for any of these types if we want to test for
    // shared_ptr, raw ptr and a value
    if (auto* casted = std::any_cast<std::shared_ptr<T>>(&value); casted != nullptr) {
      return casted->get();
    }
    if (auto* casted = std::any_cast<std::shared_ptr<const T>>(&value); casted != nullptr) {
      return casted->get();
    }
    if (auto* casted = std::any_cast<T*>(&value); casted != nullptr) {
      return *casted;
    }
    if (auto* casted = std::any_cast<const T*>(&value); casted != nullptr) {
      return *casted;
    }
    return std::any_cast<T>(&value);
  }

  template <typename T>
  static constexpr auto any_to_specific = std::views::transform([](const auto& pair) -> std::pair<std::string_view, const T*> { return { pair.first, any_cast_try_shared_ptr<T>(pair.second) }; });

  static constexpr auto filter_nullptr_in_pair = std::views::filter([](const auto& pair) { return pair.second != nullptr; });

  static constexpr auto filter_nullptr = std::views::filter([](const auto* ptr) -> bool { return ptr != nullptr; });

  static constexpr auto pair_to_reference = std::views::transform([](const auto& pair) -> const auto& { return *pair.second; });

  static constexpr auto pair_to_value = std::views::transform([](const auto& pair) { return pair.second; });

  static constexpr auto pointer_to_reference = std::views::transform([](const auto* ptr) -> auto& { return *ptr; });

  template <typename T>
  auto iterateByType() const
  {
    return mObjects | any_to_specific<T> | filter_nullptr_in_pair | pair_to_reference;
  }

  template <typename T, std::predicate<const std::pair<std::string_view, const T*>&> Pred>
  auto iterateByTypeAndFilter(Pred&& filter) const
  {
    return mObjects | any_to_specific<T> | filter_nullptr_in_pair | std::views::filter(filter) | pair_to_reference;
  }

  template <typename StoredType, typename ResultingType, std::predicate<const std::pair<std::string_view, const StoredType*>&> Pred, invocable_r<const ResultingType*, const StoredType*> Transform>
  auto iterateByTypeFilterAndTransform(Pred&& filter, Transform&& transform) const
  {
    return mObjects |
           any_to_specific<StoredType> |
           filter_nullptr_in_pair |
           std::views::filter(filter) |
           pair_to_value |
           std::views::transform(transform) |
           filter_nullptr |
           pointer_to_reference;
  }

  size_t size() const noexcept
  {
    return mObjects.size();
  }

 private:
  std::map<std::string, std::any, std::less<>> mObjects;
};

} // namespace o2::quality_control::core

#endif
