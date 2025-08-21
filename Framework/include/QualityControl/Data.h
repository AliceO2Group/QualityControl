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
#include <memory>
#include <optional>
#include <string>
#include <ranges>
#include <type_traits>

namespace o2::quality_control::core
{

template <typename Function, typename Result, typename... Args>
concept invocable_r = std::invocable<Function, Args...> && std::same_as<std::invoke_result_t<Function, Args...>, Result>;

template <typename ContainerMap>
class DataGeneric
{
 public:
  DataGeneric() = default;

  template <typename Result>
  std::optional<std::reference_wrapper<const Result>> get(std::string_view key);

  template <typename T, typename... Args>
  void emplace(std::string_view key, Args&&... args);

  template <typename T>
  void insert(std::string_view key, const T& value);

  template <typename T>
  auto iterateByType() const;

  template <typename T, std::predicate<const std::pair<std::string_view, const T*>&> Pred>
  auto iterateByTypeAndFilter(Pred&& filter) const;

  template <typename StoredType, typename ResultingType, std::predicate<const std::pair<std::string_view, const StoredType*>&> Pred, invocable_r<const ResultingType*, const StoredType*> Transform>
  auto iterateByTypeFilterAndTransform(Pred&& filter, Transform&& transform) const;

  size_t size() const noexcept;

 private:
  ContainerMap mObjects;
};

struct StringHash {
  using is_transparent = void; // Required for heterogeneous lookup

  std::size_t operator()(const std::string& str) const
  {
    return std::hash<std::string>{}(str);
  }

  std::size_t operator()(std::string_view sv) const
  {
    return std::hash<std::string_view>{}(sv);
  }
};

using transparent_unordered_map = std::unordered_map<std::string, std::any, StringHash, std::equal_to<>>;

using Data = DataGeneric<transparent_unordered_map>;

} // namespace o2::quality_control::core

#include "Data.inl"

#endif
