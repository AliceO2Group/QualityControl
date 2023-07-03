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

#include "QualityControl/Activity.h"

#include <string>
#include <unordered_map>
#include <optional>

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
  CustomParameters();

  void set(const std::string& key, const std::string& value, const std::string& runType = "default", const std::string& beamType = "default");

  /**
   * Return all the parameters (key-value pairs) for the given runType and beamType.
   * @param runType
   * @param beamType
   * @return a map of the key-value pairs
   * @throw std::out_of_range if no key-value pair correspond to these beamType and runType
   */
  const std::unordered_map<std::string, std::string>& getAllForRunBeam(const std::string& runType, const std::string& beamType);

  /**
   * Return all the parameters (key-value pairs) for the default runType and the default beamType.
   * @return
   */
  const std::unordered_map<std::string, std::string>& getAllDefaults();

  /**
   * Return the value for the given key, runType and beamType.
   * @param key
   * @param runType
   * @param beamType
   * @return the value for the given key, runType and beamType.
   * @throw std::out_of_range if no key-value pair corresponds to this key and to these beamType and runType
   */
  std::string at(const std::string& key, const std::string& runType = "default", const std::string& beamType = "default") const;

  /**
   * Return the optional value for the given key, runType and beamType (the two latter optional).
   * @param key
   * @param runType
   * @param beamType
   * @return an optional with the value for the given key, runType and beamType or empty if not found.
   */
  std::optional<std::string> atOptional(const std::string& key, const std::string& runType = "default", const std::string& beamType = "default") const;

  /**
   * Return the optional value for the given key in the specified activity.
   * @param key
   * @param activity
   * @return an optional with the value for the given key and for the given activity.
   */
  std::optional<std::string> atOptional(const std::string& key, const Activity& activity) const;

  /**
   * Return the value for the given key, runType and beamType (the two latter optional). If it does not exist, returns the default value if provided or an empty string.
   * @param key
   * @param runType
   *
   * @param beamType
   * @param defaultValue
   * @return the value for the given key, runType and beamType. If it is not found, it returns an empty string or defaultValue if provided.
   */
  std::string atOrDefaultValue(const std::string& key, std::string defaultValue = "", const std::string& runType = "default", const std::string& beamType = "default");

  /**
   * Returns the number of items found for the provided key, beamType and runType. It can only be either 0 or 1.
   * @param key
   * @param runType
   * @param beamType
   * @return 0 or 1 depending if a value is found.
   */
  int count(const std::string& key, const std::string& runType = "default", const std::string& beamType = "default") const;

  /**
   * Finds the items whose key is `key`.
   * A runType and/or a beamType can be set as well.
   * If it is not found it returns end()
   * @param key
   * @param runType
   * @param beamType
   * @return the item or end()
   */
  std::unordered_map<std::string, std::string>::const_iterator find(const std::string& key, const std::string& runType = "default", const std::string& beamType = "default") const;

  std::unordered_map<std::string, std::string>::const_iterator end() const;

  /**
   * Return the value for the given key, and for beamType=default and runType=default.
   * If the key does not exist, it will create it with a value="".
   * @param key
   * @return
   */
  std::string operator[](const std::string& key) const;

  /**
   * Assign the value to the key, and for beamType=default and runType=default.
   * @param key
   * @return
   */
  std::string& operator[](const std::string& key);

  /**
   * prints the CustomParameters
   */
  friend std::ostream& operator<<(std::ostream& out, const CustomParameters& customParameters);

 private:
  CustomParametersType mCustomParameters;
};

} // namespace o2::quality_control::core

#endif // QC_CUSTOM_PARAMETERS_H