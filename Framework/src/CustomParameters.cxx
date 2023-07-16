// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "QualityControl/CustomParameters.h"
#include <DataFormatsParameters/ECSDataAdapters.h>
#include <iostream>

namespace o2::quality_control::core
{

std::ostream& operator<<(std::ostream& out, const CustomParameters& customParameters)
{
  // todo: should we swallow the exceptions here ?
  for (const auto& runType : customParameters.mCustomParameters) {
    for (const auto& beamType : runType.second) {
      for (const auto& name : beamType.second) {
        out << runType.first << " - " << beamType.first << " - " << name.first << " : " << name.second << "\n";
      }
    }
  }
  return out;
}

CustomParameters::CustomParameters()
{
  mCustomParameters["default"]["default"] = {};
}

void CustomParameters::set(const std::string& key, const std::string& value, const std::string& runType, const std::string& beamType)
{
  mCustomParameters[runType][beamType][key] = value;
}

const std::unordered_map<std::string, std::string>& CustomParameters::getAllForRunBeam(const std::string& runType, const std::string& beamType)
{
  if (mCustomParameters.count(runType) > 0 && mCustomParameters[runType].count(beamType) > 0) {
    return mCustomParameters[runType][beamType];
  }
  throw std::out_of_range("Unknown beam or run: " + runType + ", " + beamType);
}

const std::unordered_map<std::string, std::string>& CustomParameters::getAllDefaults()
{
  return getAllForRunBeam("default", "default");
}

std::string CustomParameters::at(const std::string& key, const std::string& runType, const std::string& beamType) const
{
  return mCustomParameters.at(runType).at(beamType).at(key);
}

std::optional<std::string> CustomParameters::atOptional(const std::string& key, const std::string& runType, const std::string& beamType) const
{
  try {
    return mCustomParameters.at(runType).at(beamType).at(key);
  } catch (const std::out_of_range& exc) {
    return {};
  }
}

std::optional<std::string> CustomParameters::atOptional(const std::string& key, const Activity& activity) const
{
  // Get the proper parameter for the given activity
  const int runType = activity.mType; // get the type for this run
  // convert it to a string (via a string_view as this is what we get from O2)
  const std::string_view runTypeStringView = o2::parameters::GRPECS::RunTypeNames[runType];
  const std::string runTypeString{ runTypeStringView };
  // get the param
  return atOptional(key, runTypeString, activity.mBeamType);
}

std::string CustomParameters::atOrDefaultValue(const std::string& key, std::string defaultValue, const std::string& runType, const std::string& beamType)
{
  try {
    return mCustomParameters.at(runType).at(beamType).at(key);
  } catch (const std::out_of_range& exc) {
    return defaultValue;
  }
}

int CustomParameters::count(const std::string& key, const std::string& runType, const std::string& beamType) const
{
  try {
    at(key, runType, beamType);
  } catch (const std::out_of_range& oor) {
    return 0;
  }
  return 1;
}

std::unordered_map<std::string, std::string>::const_iterator CustomParameters::find(const std::string& key, const std::string& runType, const std::string& beamType) const
{
  auto subTreeRunType = mCustomParameters.find(runType);
  if (subTreeRunType == mCustomParameters.end()) {
    return end();
  }
  auto subTreeBeamType = subTreeRunType->second.find(beamType);
  if (subTreeBeamType == subTreeRunType->second.end()) {
    return end();
  }
  auto foundValue = subTreeBeamType->second.find(key);
  if (foundValue == subTreeBeamType->second.end()) {
    return end();
  }
  return foundValue;
}

std::unordered_map<std::string, std::string>::const_iterator CustomParameters::end() const
{
  return mCustomParameters.at("default").at("default").end();
}

std::string CustomParameters::operator[](const std::string& key) const
{
  return at(key);
}

std::string& CustomParameters::operator[](const std::string& key)
{
  if (count(key) == 0) {
    set(key, "");
  }
  return mCustomParameters.at("default").at("default").at(key);
}

} // namespace o2::quality_control::core