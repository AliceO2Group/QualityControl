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
/// \file   QCInputsFactory.h
/// \author Michal Tichak
/// \brief Factory functions to populate QCInputs from object maps.
///

#ifndef QC_CORE_QCINPUTSFACTORY_H
#define QC_CORE_QCINPUTSFACTORY_H

#include "QCInputs.h"
#include "QualityControl/MonitorObject.h"
#include "QualityObject.h"
#include <map>
#include <memory>
#include <string>

namespace o2::quality_control::core
{

/// \brief Create QCInputs from a map of MonitorObject instances.
/// \param moMap Map from name to shared MonitorObject pointer.
QCInputs createData(const std::map<std::string, std::shared_ptr<MonitorObject>>& moMap);

/// \brief Create QCInputs from a map of QualityObject instances.
/// \param qoMap Map from name to shared QualityObject pointer.
QCInputs createData(const QualityObjectsMapType& qoMap);

} // namespace o2::quality_control::core

#endif
