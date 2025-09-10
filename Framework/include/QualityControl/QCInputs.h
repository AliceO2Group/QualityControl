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
/// \file   QCInputs.h
/// \author Michal Tichak
/// \brief Generic container for heterogeneous QC input data.
///
/// \par Example
/// \code{.cpp}
/// QCInputs data;
/// auto* h1 = new TH1F("th11", "th11", 100, 0, 99);
/// data.insert("mo", std::make_shared<MonitorObject>(h1, "taskname", "class1", "TST"));
/// if (auto opt = data.get<MonitorObject>("mo")) {
///   MonitorObject& moObject = opt.value();
///   std::cout << "mo name: " << moObject.getName() << std::endl;
/// }
/// for (const auto& mo : data.iterateByType<MonitorObject>()) {
///   // process each value
/// }
/// \endcode
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

/// \brief Requires a callable to return exactly the specified type.
/// \tparam Function Callable type to invoke.
/// \tparam Result Expected return type.
/// \tparam Args Argument types for invocation.
template <typename Function, typename Result, typename... Args>
concept invocable_r = std::invocable<Function, Args...> &&
                      std::same_as<std::invoke_result_t<Function, Args...>, Result>;

/// \brief Heterogeneous storage for named QC input objects.
///
/// Stores values in an std::unordered_map<std::string, std::any> while
/// offering type-safe get, iteration, filtering, and transformation.
class QCInputs
{
 public:
  QCInputs() = default;

  /// \brief Retrieve the object stored under the given key with matching type.
  /// \tparam Result Expected stored type.
  /// \param key Identifier for the stored object.
  /// \returns Optional reference to const Result if found desired item of type Result.
  /// \par Example
  /// \code{.cpp}
  /// if (auto opt = data.get<MonitorObject>("mo")) {
  ///   if (opt.has_value()){
  ///     const unsigned& value = opt.value(); // careful about using auto here as we want to invoke implicit conversion operator of reference_wrapper
  ///   }
  /// }
  /// \endcode
  template <typename Result>
  std::optional<std::reference_wrapper<const Result>> get(std::string_view key);

  /// \brief Construct and store an object of type T under the given key.
  /// \tparam T Type to construct and store.
  /// \param key Identifier under which to store the object.
  /// \param args Arguments forwarded to T's constructor.
  /// \par Example
  /// \code{.cpp}
  /// auto* h1 = new TH1F("th11", "th11", 100, 0, 99);
  /// data.emplace<MonitorObject>("mo", h1, "taskname", "class1", "TST");
  /// \endcode
  template <typename T, typename... Args>
  void emplace(std::string_view key, Args&&... args);

  /// \brief Store a copy of value under the given key.
  /// \tparam T Type of the value to store.
  /// \param key Identifier under which to store the value.
  /// \param value Const reference to the value to insert.
  /// \par Example
  /// \code{.cpp}
  /// auto* h1 = new TH1F("th11", "th11", 100, 0, 99);
  /// data.insert("mo", std::make_shared<MonitorObject>(h1, "taskname", "class1", "TST"));
  /// \endcode
  template <typename T>
  void insert(std::string_view key, const T& value);

  /// \brief Iterate over all stored objects matching type Result.
  /// \tparam Result Type filter for iteration.
  /// \returns Range of const references to stored Result instances.
  /// \par Example
  /// \code{.cpp}
  /// for (auto& mo : data.iterateByType<MonitorObject>()) {
  ///   // use val
  /// }
  /// \endcode
  template <typename Result>
  auto iterateByType() const;

  /// \brief Iterate over stored objects of type Result satisfying a predicate.
  /// \tparam Result type filter for iteration.
  /// \tparam Pred Callable predicate on (key, Result*) pairs.
  /// \param filter Predicate to apply for filtering entries.
  /// \returns Range of const references to Result passing the filter.
  /// \par Example
  /// \code{.cpp}
  /// auto nameFilter = [](auto const& pair) { return pair.second->getName() == "name"; };
  /// for (auto& mo : data.iterateByTypeAndFilter<MonitorObject>(nameFilter)) {
  ///   // use mo
  /// }
  /// \endcode
  template <typename Result, std::predicate<const std::pair<std::string_view, const Result*>&> Pred>
  auto iterateByTypeAndFilter(Pred&& filter) const;

  /// \brief Filter entries of type Stored, then transform to type Result.
  /// \tparam Stored Original stored type for filtering.
  /// \tparam Result Target type after transformation.
  /// \tparam Pred Callable predicate on (key, Stored*) pairs.
  /// \tparam Transform Callable transforming Stored* to Result*.
  ///         This Callable can return nullptr but it will be filtered out
  ///         from results
  /// \param filter Predicate to apply before transformation.
  /// \param transform Callable to convert Stored to Result.
  /// \returns Range of const references to resulting objects.
  /// \par Example
  /// \code{.cpp}
  /// // if we stored some MOs that are not TH1F, these will be filtered out of results
  /// auto toHistogram = [](auto const& p) -> const auto* { return dynamic_cast<TH1F*>(p.second->getObject()); };
  /// auto nameFilter = [](auto const& p){ return p.first == "histo"; };
  /// for (auto& h : data.iterateByTypeFilterAndTransform<MonitorObject, TH1F>(nameFilter, toHistogram)) {
  ///   // use histogram h
  /// }
  /// \endcode
  template <typename Stored, typename Result, std::predicate<const std::pair<std::string_view, const Stored*>&> Pred, invocable_r<const Result*, const Stored*> Transform>
  auto iterateByTypeFilterAndTransform(Pred&& filter, Transform&& transform) const;

  /// \brief Number of stored entries.
  /// \returns Size of the underlying container.
  /// \par Example
  /// \code{.cpp}
  /// size_t n = data.size();
  /// \endcode
  size_t size() const noexcept;

 private:
  /// \brief Transparent hash functor for string and string_view.
  ///
  /// Enables heterogeneous lookup in unordered maps keyed by std::string.
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

  std::unordered_map<std::string, std::any, StringHash, std::equal_to<>> mObjects;
};

} // namespace o2::quality_control::core

#include "QCInputs.inl"

#endif
