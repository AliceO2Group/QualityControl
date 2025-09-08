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
/// \file   QCInputsAdapters.h
/// \author Michal Tichak
/// \brief Adapters to build and query QCInputs from Monitor and Quality objects.
///
/// \par Example
/// \code{.cpp}
/// // Iterate monitor objects by task name
/// for (const auto& mo : iterateMonitorObjects(data, "task1")) {
///   // use mo
/// }
/// // Retrieve a specific MonitorObject
/// if (auto opt = getMonitorObject(data, "objName", "task1")) {
///   const auto& mo = opt->get();
///   // use mo
/// }
/// // Iterate and retrieve quality objects
/// for (const auto& qo : iterateQualityObjects(data)) {
///   // use qo
/// }
/// if (auto qoOpt = getQualityObject(data, "check1")) {
///   if(qoOpt.has_value()){
///     QualityObject& qo = qoOpt->value();
///     // ...
///   }
///   // use qoOpt->get()
/// }
/// \endcode
///

#ifndef QC_CORE_DATA_ADAPTERS_H
#define QC_CORE_DATA_ADAPTERS_H

// Core inputs and adapters
#include "QCInputs.h"
#include "QualityControl/MonitorObject.h"
#include "QualityObject.h"
#include "QCInputsFactory.h"

namespace o2::quality_control::core
{

/// \brief Iterate over all MonitorObject entries in QCInputs.
inline auto iterateMonitorObjects(const QCInputs& data);

/// \brief Iterate over MonitorObject entries filtered by task name.
/// \param data QCInputs containing MonitorObjects.
/// \param taskName Task name to filter entries.
inline auto iterateMonitorObjects(const QCInputs& data, std::string_view taskName);

/// \brief Retrieve the first MonitorObject of type StoredType matching both name and task.
/// \tparam StoredType Type of MonitorObject or stored class to retrieve.
/// \param data QCInputs to search.
/// \param objectName Name of the MonitorObject.
/// \param taskName Name of the originating task.
/// \returns Optional reference to const StoredType if found.
template <typename StoredType = MonitorObject>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const QCInputs& data, std::string_view objectName, std::string_view taskName);

// returns first occurence of MO with given name (possible name clash)
/// \brief Retrieve the first MonitorObject of type StoredType matching name.
/// \tparam StoredType Type of MonitorObject or stored class to retrieve.
/// \param data QCInputs to search.
/// \param objectName Name of the MonitorObject.
/// \returns Optional reference to const StoredType if found.
template <typename StoredType = MonitorObject>
std::optional<std::reference_wrapper<const StoredType>> getMonitorObject(const QCInputs& data, std::string_view objectName);

/// \brief Iterate over all QualityObject entries in QCInputs.
inline auto iterateQualityObjects(const QCInputs& data);

/// \brief Iterate over QualityObject entries filtered by check name.
/// \param data QCInputs containing QualityObjects.
/// \param checkName Check name to filter entries.
inline auto iterateQualityObjects(const QCInputs& data, std::string_view checkName);

/// \brief Retrieve the first QualityObject matching a given check name.
/// \param data QCInputs to search.
/// \param checkName Name of the quality check.
/// \returns Optional reference to const QualityObject if found.
std::optional<std::reference_wrapper<const QualityObject>> getQualityObject(const QCInputs& data, std::string_view checkName);

} // namespace o2::quality_control::core

// Templates definitions
#include "QCInputsAdapters.inl"

#endif
