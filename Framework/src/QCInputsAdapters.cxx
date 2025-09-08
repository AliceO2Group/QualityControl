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
/// \file   QCInputsAdapters.cxx
/// \author Michal Tichak
///

#include "QualityControl/QCInputsAdapters.h"

namespace o2::quality_control::core
{

std::optional<std::reference_wrapper<const QualityObject>> getQualityObject(const QCInputs& data, std::string_view objectName)
{
  const auto filterQOByName = [objectName](const auto& pair) {
    return std::string_view(pair.second->getCheckName()) == objectName;
  };
  for (const auto& qo : data.iterateByTypeAndFilter<QualityObject>(filterQOByName)) {
    return { qo };
  }
  return std::nullopt;
}

} // namespace o2::quality_control::core
