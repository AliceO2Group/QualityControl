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
/// \file   DataAdapters.inl
/// \author Michal Tichak
///

#ifndef QC_CORE_DATA_ADAPTERS_IMPL_H
#define QC_CORE_DATA_ADAPTERS_IMPL_H

#include <optional>
#include <string_view>
#include "QualityControl/Data.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"

#ifdef QC_CORE_DATA_ADAPTERS_H
#include "QualityControl/DataAdapters.h"
#endif

namespace o2::quality_control::core
{

namespace helpers
{

}

template <typename Result, typename DataContainer>
auto iterateMonitorObjects(const DataGeneric<DataContainer>& data, std::string_view moName)
{
  return data.template iterateByTypeFilterAndTransform<MonitorObject, Result>(
    [name = std::string{ moName }](const std::pair<std::string_view, const MonitorObject*>& pair) -> bool { return std::string_view{ pair.second->GetName() } == name; },
    [](const MonitorObject* ptr) -> const Result* { return dynamic_cast<const Result*>(ptr->getObject()); });
}

inline auto iterateMonitorObjects(const o2::quality_control::core::Data& data)
{
  return data.iterateByType<o2::quality_control::core::MonitorObject>();
}

inline auto iterateMonitorObjects(const Data& data, std::string_view taskName)
{
  const auto filterMOByTaskName = [taskName](const auto& pair) {
    return pair.second->getTaskName() == taskName;
  };

  return data.iterateByTypeAndFilter<MonitorObject>(filterMOByTaskName);
}

namespace helpers
{

template <typename StoredType, typename Filter>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObjectCommon(const Data& data, Filter&& filter)
{
  if constexpr (std::same_as<StoredType, MonitorObject>) {
    for (const auto& mo : data.iterateByTypeAndFilter<o2::quality_control::core::MonitorObject>(filter)) {
      return { mo };
    }
  } else {
    const auto getInternalObject = [](const MonitorObject* ptr) -> const auto* {
      return dynamic_cast<const StoredType*>(ptr->getObject());
    };
    for (const auto& v : data.template iterateByTypeFilterAndTransform<MonitorObject, StoredType>(filter, getInternalObject)) {
      return { v };
    }
  }
  return std::nullopt;
}

}; // namespace helpers

template <typename StoredType>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const Data& data, std::string_view objectName, std::string_view taskName)
{

  const auto filterMOByNameAndTaskName = [objectName, taskName](const auto& pair) {
    return std::tuple{ std::string_view{ pair.second->GetName() }, pair.second->getTaskName() } == std::tuple{ objectName, taskName };
  };

  return helpers::getMonitorObjectCommon<StoredType>(data, filterMOByNameAndTaskName);
}

template <typename StoredType>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const Data& data, std::string_view objectName)
{
  const auto filterMOByName = [objectName](const auto& pair) {
    return std::string_view(pair.second->GetName()) == objectName;
  };

  return helpers::getMonitorObjectCommon<StoredType>(data, filterMOByName);
}

inline auto iterateQualityObjects(const Data& data)
{
  return data.iterateByType<o2::quality_control::core::QualityObject>();
}

} // namespace o2::quality_control::core

#endif
