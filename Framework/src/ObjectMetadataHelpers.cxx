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
/// \file   ObjectMetadataHelpers.cxx
/// \author Michal Tichak
///

#include <charconv>
#include "QualityControl/ObjectMetadataHelpers.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control::repository
{
std::optional<unsigned long> parseCycle(const std::string& cycleStr)
{
  unsigned long cycleVal{};
  if (auto parse_res = std::from_chars(cycleStr.c_str(), cycleStr.c_str() + cycleStr.size(), cycleVal);
      parse_res.ec != std::errc{}) {
    ILOG(Warning, Support) << "failed to decypher " << repository::metadata_keys::cycleNumber << " metadata with value " << cycleStr
                           << ", with std::errc " << std::make_error_code(parse_res.ec).message() << ENDM;
    return std::nullopt;
  }
  return cycleVal;
}
} // namespace o2::quality_control::repository
