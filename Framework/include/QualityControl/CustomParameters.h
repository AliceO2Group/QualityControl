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

class CustomParameters
{
  // runtype -> beamtype -> key -> value
  using CustomParametersType = std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::string>>>;

 public:
  CustomParameters()
  {
    mCustomParameters["default"]["default"] = {};
  }

  void set(const std::string& value, const std::string& name, const std::string& runType = "default", const std::string& beamType = "default")
  {
    mCustomParameters[runType][beamType][name] = value;
  }

  const CustomParametersType& getCustomParameters() const
  {
    return mCustomParameters;
  }

  /**
   *
   * @param runType
   * @param beamType
   * @return
   * @throw std::out_of_range
   */
  const std::unordered_map<std::string, std::string>& getAllForRunBeam(const std::string& runType, const std::string& beamType)
  {
    if (mCustomParameters.count(runType) > 0 && mCustomParameters[runType].count(beamType) > 0) {
      return mCustomParameters[runType][beamType];
    }
    throw std::out_of_range("Unknown beam or run: " + runType + ", " + beamType);
  }

  /**
   *
   * @return
   * @throw std::out_of_range
   */
  const std::unordered_map<std::string, std::string>& getAllDefaults()
  {
    return getAllForRunBeam("default", "default");
  }

  /**
   *
   * @param name
   * @param runType
   * @param beamType
   * @return
   * @throw std::out_of_range
   */
  std::string at(const std::string& name, const std::string& runType = "default", const std::string& beamType = "default") const
  {
    return mCustomParameters.at(runType).at(beamType).at(name);
  }

  std::string atOrDefaultValue(const std::string& name, const std::string& runType, const std::string& beamType, std::string defaultValue) const
  {
    try {
      return mCustomParameters.at(runType).at(beamType).at(name);
    } catch (const std::out_of_range& exc) {
      return defaultValue;
    }
  }

  std::string atOrDefaultValue(const std::string& name, std::string defaultValue) const
  {
    try {
      return mCustomParameters.at("default").at("default").at(name);
    } catch (const std::out_of_range& exc) {
      return defaultValue;
    }
  }

  /**
   *
   * @param name
   * @param runType
   * @param beamType
   * @return
   */
  int count(const std::string& name, const std::string& runType = "default", const std::string& beamType = "default") const
  {
    try {
      at(name, runType, beamType);
    } catch (const std::out_of_range& oor) {
      return 0;
    }
    return 1;
  }

  /**
   * Finds the items whose key is `name`.
   * A runType and/or a beamType can be set as well.
   * If it is not found it returns end()
   * @param name
   * @param runType
   * @param beamType
   * @return the item or end()
   */
  std::unordered_map<std::string, std::string>::const_iterator find(const std::string& name, const std::string& runType = "default", const std::string& beamType = "default") const
  {
    auto subTreeRunType = mCustomParameters.find(runType);
    if (subTreeRunType == mCustomParameters.end()) {
      return end();
    }
    auto subTreeBeamType = subTreeRunType->second.find(beamType);
    if (subTreeBeamType == subTreeRunType->second.end()) {
      return end();
    }
    auto foundValue = subTreeBeamType->second.find(name);
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
   *
   * @param index
   * @return
   * @throw std::out_of_range
   */
  std::string operator[](const std::string& index) const
  {
    return at(index);
  }

  /// prints the CustomParameters
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