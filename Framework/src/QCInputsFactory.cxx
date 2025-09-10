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
/// \file   QCInputsFactory.cxx
/// \author Michal Tichak
///

#include "QualityControl/QCInputsFactory.h"

namespace o2::quality_control::core
{

QCInputs createData(const std::map<std::string, std::shared_ptr<MonitorObject>>& moMap)
{
  QCInputs data;
  for (const auto& [key, mo] : moMap) {
    data.insert(key, mo);
  }
  return data;
}

QCInputs createData(const QualityObjectsMapType& qoMap)
{
  QCInputs data;
  for (const auto& [key, qo] : qoMap) {
    data.insert(key, qo);
  }
  return data;
}

} // namespace o2::quality_control::core
