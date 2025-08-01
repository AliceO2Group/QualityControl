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
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <typeinfo>

namespace o2::quality_control::core
{

class MonitorObject;

class Data
{
  // change this for boost::flat_map? or do a magic and split by different keys?
  using InternalContainer = std::map<std::string, std::any, std::less<>>;
  using InternalContainerConstIterator = InternalContainer::const_iterator;

 public:
  template <typename T>
  class Iterator
  {
   public:
    using value_type = std::pair<std::reference_wrapper<const std::string>, std::reference_wrapper<const T>>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using reference = const value_type&;
    using pointer = const value_type*;

    Iterator() = default;
    Iterator(InternalContainerConstIterator it, InternalContainerConstIterator end)
      : mIt{ it }, mEnd{ end }, val{ std::nullopt }
    {
      seek_next_valid(mIt);
    }

    reference operator*() const
    {
      return val.value();
    }

    pointer operator->() const
    {
      return &val.value();
    }

    Iterator& operator++()
    {
      // can this be optimised out?
      if (mIt != mEnd) {
        seek_next_valid(++mIt);
      }
      return *this;
    }

    Iterator operator++(int)
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator==(const Iterator& other) const
    {
      return mIt == other.mIt;
    }

   private:
    InternalContainerConstIterator mIt;
    InternalContainerConstIterator mEnd;
    std::optional<value_type> val;

    void seek_next_valid(InternalContainerConstIterator startingIt)
    {
      for (auto it = startingIt; it != mEnd; ++it) {
        if (auto* casted = std::any_cast<T>(&it->second); casted != nullptr) {
          val.emplace(it->first, *casted);
          break;
        }
      }
    }
  };

  static_assert(std::forward_iterator<Iterator<int>>);

  Data() = default;
  Data(const std::map<std::string, std::shared_ptr<MonitorObject>>& moMap);

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

  template <typename StoreType>
  void insert(std::string_view key, const StoreType& value)
  {
    mObjects.insert({ std::string{ key }, value });
  }

  template <typename Type>
  std::vector<Type> getAllOfType() const
  {
    std::vector<Type> result;
    for (const auto& [_, object] : mObjects) {
      if (auto* casted = std::any_cast<Type>(&object); casted != nullptr) {
        result.push_back(*casted);
      }
    }
    return result;
  }

  template <typename Type, std::predicate<Type> Pred>
  std::vector<Type> getAllOfTypeIf(Pred filter) const
  {
    std::vector<Type> result;
    for (const auto& [_, object] : mObjects) {
      if (auto* casted = std::any_cast<Type>(&object); casted != nullptr) {
        if (filter(*casted)) {
          result.push_back(*casted);
        }
      }
    }
    return result;
  }

  template <typename T>
  Iterator<T> begin()
  {
    return { mObjects.begin(), mObjects.end() };
  }

  template <typename T>
  Iterator<T> end()
  {
    return { mObjects.end(), mObjects.end() };
  }

 private:
  InternalContainer mObjects;
};

} // namespace o2::quality_control::core

#endif
