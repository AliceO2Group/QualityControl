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
/// \file   DataAdapters.h
/// \author Michal Tichak
///

#ifndef QC_CORE_DATA_ADAPTERS_H
#define QC_CORE_DATA_ADAPTERS_H

#include "Data.h"
#include "QualityControl/MonitorObject.h"
#include "QualityObject.h"

namespace o2::quality_control::core
{

Data createData(const std::map<std::string, std::shared_ptr<MonitorObject>>& moMap);
Data createData(const QualityObjectsMapType& moMap);

template <typename Result, typename DataContainer>
auto iterateMOsFilterByNameAndTransform(const DataGeneric<DataContainer>& data, std::string_view moName)
{
  return data.template iterateByTypeFilterAndTransform<MonitorObject, Result>(
    [name = std::string{ moName }](const std::pair<std::string_view, const MonitorObject*>& pair) -> bool { return std::string_view{ pair.second->GetName() } == name; },
    [](const MonitorObject* ptr) -> const Result* { return dynamic_cast<const Result*>(ptr->getObject()); });
}

} // namespace o2::quality_control::core

#endif
