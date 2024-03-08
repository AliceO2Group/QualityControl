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

#include <iostream>
#include <sstream>

#include "QualityControl/HashDataDescription.h"

namespace o2::common::hash
{

// creates hash of input string and returns hexadecimal representation
auto to_hexa(const std::string& input, size_t hash_length) -> std::string
{
  std::stringstream ss;
  ss << std::hex << std::hash<std::string>{}(input);
  return std::move(ss).str().substr(0, hash_length);
};

o2::header::DataDescription createDataDescription(const std::string& name, size_t hashLength)
{
  o2::header::DataDescription description{};
  if (name.size() <= o2::header::DataDescription::size) {
    description.runtimeInit(name.c_str());
    return description;
  } else {

    std::stringstream ss{};
    ss << std::hex << std::hash<std::string>{}(name);

    description.runtimeInit(name.substr(0, o2::header::DataDescription::size - hashLength).append(hash::to_hexa(name, hashLength)).c_str());
    std::cout << description.str << "\n";
    return description;
  }
}

} // namespace o2::common::hash
