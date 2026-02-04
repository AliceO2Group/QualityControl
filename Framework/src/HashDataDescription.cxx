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
#include "QualityControl/QcInfoLogger.h"

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
auto toHex(const std::string& input, size_t hashLength) -> std::string
{
  std::stringstream ss;
  ss << std::setfill('0') << std::left << std::setw(hashLength) << std::noshowbase << std::hex << djb2(input);
  return std::move(ss).str().substr(0, hashLength);
};

} // namespace hash


header::DataOrigin createDataOrigin(char actorTypeCharacterId, const std::string& detectorCode)
{
  std::string originStr{ actorTypeCharacterId };
  if (detectorCode.empty()) {
    // fixme: that's probably a configuration error, so maybe we should just throw?
    ILOG(Warning, Support) << "empty detector code for a task data origin, trying to survive with: DET" << ENDM;
    originStr += "DET";
  } else if (detectorCode.size() > 3) {
    ILOG(Warning, Support) << "too long detector code for a task data origin: " + detectorCode + ", trying to survive with: " + detectorCode.substr(0, 3) << ENDM;
    originStr += detectorCode.substr(0, 3);
  } else {
    originStr += detectorCode;
  }
  o2::header::DataOrigin origin;
  origin.runtimeInit(originStr.c_str());
  return origin;
}

std::string createDescriptionWithHash(const std::string& input, size_t hashLength)
{
  return input.substr(0, o2::header::DataDescription::size - hashLength).append(hash::toHex(input, hashLength));
}

auto createDataDescription(const std::string& name, size_t hashLength) -> o2::header::DataDescription
{
  o2::header::DataDescription description{};

  if (name.size() <= o2::header::DataDescription::size) {
    description.runtimeInit(name.c_str());
    return description;
  } else {
    const auto descriptionWithHash = createDescriptionWithHash(name, hashLength);
    ILOG(Debug, Devel) << "Too long data description name [" << name << "] changed to [" << descriptionWithHash << "]" << ENDM;
    description.runtimeInit(descriptionWithHash.c_str());
    return description;
  }
}

} // namespace o2::quality_control::core
