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

#include <any>
#include <concepts>
#include <iostream>
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
 public:
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
    // std::cout << "inserting value: " << value << ", of type: " << typeid(StoreType).name() << "\n";
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

 private:
  std::map<std::string, std::any, std::less<>> mObjects;
};

} // namespace o2::quality_control::core
