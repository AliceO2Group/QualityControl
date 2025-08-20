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

QCInputs createData(const std::map<std::string, std::shared_ptr<MonitorObject>>& moMap);
QCInputs createData(const QualityObjectsMapType& moMap);

inline auto iterateMonitorObjects(const QCInputs& data);
inline auto iterateMonitorObjects(const QCInputs& data, std::string_view taskName);

template <typename StoredType = MonitorObject>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const QCInputs& data, std::string_view objectName, std::string_view taskName);

// returns first occurence of MO with given name (possible name clash)
template <typename StoredType = MonitorObject>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const QCInputs& data, std::string_view objectName);

inline auto iterateQualityObjects(const QCInputs& data);

std::optional<std::reference_wrapper<const QualityObject>> getQualityObject(const QCInputs& data, std::string_view checkName);

} // namespace o2::quality_control::core

// Templates definitions
#include "QualityControl/DataAdapters.inl"

#endif
