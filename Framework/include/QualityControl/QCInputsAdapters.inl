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
/// \file   QCInputsAdapters.inl
/// \author Michal Tichak
///

#ifndef QC_CORE_DATA_ADAPTERS_INL
#define QC_CORE_DATA_ADAPTERS_INL

#include <optional>
#include <string_view>
#include "QCInputs.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"

namespace o2::quality_control::core
{

inline auto iterateMonitorObjects(const o2::quality_control::core::QCInputs& data)
{
  return data.iterateByType<o2::quality_control::core::MonitorObject>();
}

inline auto iterateMonitorObjects(const QCInputs& data, std::string_view taskName)
{
  const auto filterMOByTaskName = [taskName](const auto& pair) {
    return pair.second->getTaskName() == taskName;
  };

  return data.iterateByTypeAndFilter<MonitorObject>(filterMOByTaskName);
}

namespace helpers
{

template <typename StoredType, typename Filter>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObjectCommon(const QCInputs& data, Filter&& filter)
{
  if constexpr (std::same_as<StoredType, MonitorObject>) {
    for (const auto& mo : data.iterateByTypeAndFilter<o2::quality_control::core::MonitorObject>(filter)) {
      return { mo };
    }
  } else {
    const auto getInternalObject = [](const MonitorObject* ptr) -> const auto* {
      return dynamic_cast<const StoredType*>(ptr->getObject());
    };
    for (const auto& v : data.iterateByTypeFilterAndTransform<MonitorObject, StoredType>(filter, getInternalObject)) {
      return { v };
    }
  }
  return std::nullopt;
}

} // namespace helpers

template <typename StoredType>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const QCInputs& data, std::string_view objectName, std::string_view taskName)
{
  const auto filterMOByNameAndTaskName = [objectName, taskName](const auto& pair) {
    return std::tuple{ std::string_view{ pair.second->GetName() }, pair.second->getTaskName() } == std::tuple{ objectName, taskName };
  };

  return helpers::getMonitorObjectCommon<StoredType>(data, filterMOByNameAndTaskName);
}

template <typename StoredType>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const QCInputs& data, std::string_view objectName)
{
  const auto filterMOByName = [objectName](const auto& pair) {
    return std::string_view(pair.second->GetName()) == objectName;
  };

  return helpers::getMonitorObjectCommon<StoredType>(data, filterMOByName);
}

inline auto iterateQualityObjects(const QCInputs& data)
{
  return data.iterateByType<o2::quality_control::core::QualityObject>();
}

inline auto iterateQualityObjects(const QCInputs& data, std::string_view checkName)
{
  const auto filterQOByName = [checkName](const auto& pair) {
    return std::string_view(pair.second->getCheckName()) == checkName;
  };
  return data.iterateByTypeAndFilter<QualityObject>(filterQOByName);
}

} // namespace o2::quality_control::core

#endif
