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

///
/// \file   CustomParameters.h
/// \author Barthelemy von Haller
///

#ifndef QC_CUSTOM_PARAMETERS_H
#define QC_CUSTOM_PARAMETERS_H

#include <string>
#include <unordered_map>
#include <iostream>

namespace o2::quality_control::core
{

/**
 * This class represents the parameters provided by the users in their config file.
 * A value can be defined for a specific run type and/or beam type. It can also be set for any
 * run type or beam type, in such a case we use the special runType and beamType `default`.
 * We expect the strings runType and beamType to correspond to what is provided by the Bookkeeping.
 *
 * Example:
 *   CustomParameters cp;
 *   cp.set("key", "value"); // for the default run and beam type
 *   cout << "value for key `key` : " << cp.at("key") << endl;
 *
 *   CustomParameters cp2;
 *   cp2.set("key", "value_run1_beam1", "physics", "pp");
 *   cout << "value for key `key` : " << cp.at("key", "physics", "pp") << endl;
 *
 */
class CustomParameters
{
  // runtype -> beamtype -> key -> value
  using CustomParametersType = std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::string>>>;

 public:
  CustomParameters()
  {
    mCustomParameters["default"]["default"] = {};
  }

  void set(const std::string& key, const std::string& value, const std::string& runType = "default", const std::string& beamType = "default")
  {
    mCustomParameters[runType][beamType][key] = value;
  }

  /**
   * Return all the parameters (key-value pairs) for the given runType and beamType.
   * @param runType
   * @param beamType
   * @return a map of the key-value pairs
   * @throw std::out_of_range if no key-value pair correspond to these beamType and runType
   */
  const std::unordered_map<std::string, std::string>& getAllForRunBeam(const std::string& runType, const std::string& beamType)
  {
    if (mCustomParameters.count(runType) > 0 && mCustomParameters[runType].count(beamType) > 0) {
      return mCustomParameters[runType][beamType];
    }
    throw std::out_of_range("Unknown beam or run: " + runType + ", " + beamType);
  }

  /**
   * Return all the parameters (key-value pairs) for the default runType and the default beamType.
   * @return
   */
  const std::unordered_map<std::string, std::string>& getAllDefaults()
  {
    return getAllForRunBeam("default", "default");
  }

  /**
   * Return the value for the given key, runType and beamType.
   * @param key
   * @param runType
   * @param beamType
   * @return the value for the given key, runType and beamType.
   * @throw std::out_of_range if no key-value pair corresponds to this key and to these beamType and runType
   */
  std::string at(const std::string& key, const std::string& runType = "default", const std::string& beamType = "default") const
  {
    return mCustomParameters.at(runType).at(beamType).at(key);
  }

  /**
   * Return the value for the given key, runType and beamType. If it does not exist, returns the provided default value.
   * @param key
   * @param runType
   * @param beamType
   * @param defaultValue
   * @return the value for the given key, runType and beamType or defaultValue if none is found.
   */
  std::string atOrDefaultValue(const std::string& key, const std::string& runType, const std::string& beamType, std::string defaultValue) const
  {
    try {
      return mCustomParameters.at(runType).at(beamType).at(key);
    } catch (const std::out_of_range& exc) {
      return defaultValue;
    }
  }

  /**
   * Return the value for the given key. If it does not exist, returns the provided default value.
   * @param key
   * @param defaultValue
   * @return the value for the given key. If it does not exist, returns the provided default value.
   */
  std::string atOrDefaultValue(const std::string& key, std::string defaultValue) const
  {
    try {
      return mCustomParameters.at("default").at("default").at(key);
    } catch (const std::out_of_range& exc) {
      return defaultValue;
    }
  }

  /**
   * Returns the number of items found for the provided key, beamType and runType. It can only be either 0 or 1.
   * @param key
   * @param runType
   * @param beamType
   * @return 0 or 1 depending if a value is found.
   */
  int count(const std::string& key, const std::string& runType = "default", const std::string& beamType = "default") const
  {
    try {
      at(key, runType, beamType);
    } catch (const std::out_of_range& oor) {
      return 0;
    }
    return 1;
  }

  /**
   * Finds the items whose key is `name`.
   * A runType and/or a beamType can be set as well.
   * If it is not found it returns end()
   * @param key
   * @param runType
   * @param beamType
   * @return the item or end()
   */
  std::unordered_map<std::string, std::string>::const_iterator find(const std::string& key, const std::string& runType = "default", const std::string& beamType = "default") const
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

  std::unordered_map<std::string, std::string>::const_iterator end() const
  {
    return mCustomParameters.at("default").at("default").end();
  }

  /**
   * Return the value for the given key, and for beamType=default and runType=default.
   * @param key
   * @return
   * @throw std::out_of_range
   */
  std::string operator[](const std::string& key) const
  {
    return at(key);
  }

  /**
   * prints the CustomParameters
   */
  friend std::ostream& operator<<(std::ostream& out, const CustomParameters& customParameters)
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

 private:
  CustomParametersType mCustomParameters;
};

} // namespace o2::quality_control::core

#endif // QC_CUSTOM_PARAMETERS_H