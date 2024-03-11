// Copyright 2019-2024 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include <iomanip>
#include <sstream>

#include "QualityControl/HashDataDescription.h"

namespace o2::quality_control::core
{

namespace hash
{

// djb2 is used instead of std::hash<std::string> to be consistent over different architectures
auto djb2(const std::string& input) -> size_t
{
  size_t hash = 5381;
  for (const auto c : input) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

// creates hash of input string and returns hexadecimal representation
// if created hash has smaller amount of digits than requested, required number of zeros is appended
auto to_hex(const std::string& input, size_t hashLength) -> std::string
{
  std::stringstream ss;
  ss << std::setfill('0') << std::left << std::setw(hashLength) << std::noshowbase << std::hex << djb2(input);
  return std::move(ss).str().substr(0, hashLength);
};

} // namespace hash

auto createDataDescription(const std::string& name, size_t hashLength) -> o2::header::DataDescription
{
  o2::header::DataDescription description{};

  if (name.size() <= o2::header::DataDescription::size) {
    description.runtimeInit(name.c_str());
    return description;
  } else {
    description.runtimeInit(name.substr(0, o2::header::DataDescription::size - hashLength).append(hash::to_hex(name, hashLength)).c_str());
    return description;
  }
}

} // namespace o2::quality_control::core
