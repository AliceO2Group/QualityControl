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
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace o2::quality_control::core
{

class MonitorObject;

class Data
{
  // change this for boost::flat_map? or do a magic and split by different keys?
  using InternalContainer = std::map<std::string, std::any, std::less<>>;
  using InternalContainerConstIterator = InternalContainer::const_iterator;

  template <typename T>
  using value_type = std::pair<std::reference_wrapper<const std::string>, std::reference_wrapper<const T>>;

  template <typename T>
  struct NoPredicateIncrementPolicy {
    bool increment(const InternalContainerConstIterator& it, std::optional<value_type<T>>& val)
    {
      if (auto* casted = std::any_cast<T>(&it->second); casted != nullptr) {
        val.emplace(it->first, *casted);
        return true;
      }
      return false;
    }
  };

  template <typename T, std::predicate<const T&> Pred>
  struct PredicateIncrementPolicy {
    Pred filter;
    bool increment(const InternalContainerConstIterator& it, std::optional<value_type<T>>& val)
    {
      if (auto* casted = std::any_cast<T>(&it->second); casted != nullptr) {
        if (filter(*casted)) {
          val.emplace(it->first, *casted);
          return true;
        }
      }
      return false;
    }
  };

 public:
  template <typename T, typename IncrementalPolicy = NoPredicateIncrementPolicy<T>>
  class Iterator
  {
   public:
    using value_type = std::pair<std::reference_wrapper<const std::string>, std::reference_wrapper<const T>>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using reference = const value_type&;
    using pointer = const value_type*;

   private:
   public:
    Iterator() = default;
    Iterator(const InternalContainerConstIterator& it, const InternalContainerConstIterator& end, const IncrementalPolicy& policy = {})
      : mIt{ it }, mEnd{ end }, mVal{ std::nullopt }
    {
      seek_next_valid(mIt);
    }

    reference operator*() const
    {
      return mVal.value();
    }

    pointer operator->() const
    {
      return &mVal.value();
    }

    Iterator& operator++()
    {
      // TODO: can this check be optimised out?
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
    IncrementalPolicy mPolicy;
    std::optional<value_type> mVal;

    void seek_next_valid(InternalContainerConstIterator& startingIt)
    {
      for (; startingIt != mEnd; ++startingIt) {
        if (mPolicy.increment(startingIt, mVal)) {
          break;
        }
      }
    }
  };

  template <typename T, std::predicate<const T&> P>
  using FilteringIterator = Iterator<T, PredicateIncrementPolicy<T, P>>;

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

  template <typename T>
  struct Range {
    Iterator<T> begin_it;
    Iterator<T> end_it;
    Iterator<T> begin() { return begin_it; }
    Iterator<T> end() { return end_it; }
  };

  template <typename T>
  Range<T> iterate() const noexcept
  {
    return {
      .begin_it = begin<T>(),
      .end_it = end<T>()
    };
  }

  template <typename T, std::predicate<const T&> P>
  struct FilteredRange {
    FilteringIterator<T, P> begin_it;
    FilteringIterator<T, P> end_it;
    FilteringIterator<T, P> begin() { return begin_it; }
    FilteringIterator<T, P> end() { return end_it; }
  };

  template <typename Type, std::predicate<const Type&> Pred>
  FilteredRange<Type, Pred> iterateAndFilter(Pred&& filter) const noexcept
  {
    return FilteredRange<Type, Pred>{
      .begin_it = begin<Type, Pred>(std::forward<Pred>(filter)),
      .end_it = end<Type, Pred>(std::forward<Pred>(filter))
    };
  }

  size_t size();

  template <typename T>
  Iterator<T> begin() const noexcept
  {
    return { mObjects.begin(), mObjects.end() };
  }

  template <typename T, std::predicate<const T&> P>
  FilteringIterator<T, P> begin(P&& filter) const noexcept
  {
    return { mObjects.begin(), mObjects.end(), PredicateIncrementPolicy<T, P>(filter) };
  }

  template <typename T>
  Iterator<T> end() const noexcept
  {
    return { mObjects.end(), mObjects.end() };
  }

  template <typename T, std::predicate<const T&> P>
  FilteringIterator<T, P> end(P&& filter) const noexcept
  {
    return { mObjects.end(), mObjects.end(), PredicateIncrementPolicy<T, P>(filter) };
  }

  template <typename Base, typename Derived>
  static std::optional<Derived*> downcast(const Base* base)
  {
    if (const auto* casted = dynamic_cast<Derived*>(base); casted != nullptr) {
      return { casted };
    }
    return std::nullopt;
  }

 private:
  InternalContainer mObjects;
};

} // namespace o2::quality_control::core

#endif
