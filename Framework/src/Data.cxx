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

#include "QualityControl/Data.h"

namespace o2::quality_control::core
{
Data::Data(const std::map<std::string, std::shared_ptr<MonitorObject>>& moMap)
{
  for (const auto& [key, mo] : moMap) {
    insert(key, mo);
  }
}
} // namespace o2::quality_control::core
