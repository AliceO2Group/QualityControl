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
/// \file   Utils.h
///

#ifndef QC_MODULE_COMMON_UTILS_H
#define QC_MODULE_COMMON_UTILS_H

#include <string>
#include <unordered_map>

#include <Framework/Logger.h>
#include "QualityControl/CustomParameters.h"

namespace o2::quality_control_modules::common
{

namespace internal
{
/// \brief Parses a string to a type
/// \param params String to be cast
/// \return Returns requested type from param
template <typename T>
T stringToType(const std::string& param)
{
  T retVal{};
  if constexpr (std::is_same<int, T>::value) {
    retVal = std::stoi(param);
  } else if constexpr (std::is_same<std::string, T>::value) {
    retVal = param;
  } else if constexpr (std::is_same<float, T>::value) {
    retVal = std::stof(param);
  } else if constexpr (std::is_same<double, T>::value) {
    retVal = std::stod(param);
  } else if constexpr (std::is_same<bool, T>::value) {
    if ((param == "true") || (param == "True") || (param == "TRUE") || (param == "1")) {
      retVal = true;
    } else if ((param == "false") || (param == "False") || (param == "FALSE") || (param == "0")) {
      retVal = false;
    } else {
      LOG(error) << "Cannot parse boolean.";
    }
  } else {
    LOG(error) << "Template type not supported";
  }
  return retVal;
}
} // namespace internal

/// \brief Gets a task parameter from the config file
/// Convenience function to return a value for a taskParameter given in the config file
/// \param params mCustomParameters from within the QualityControl/TaskInterface
/// \param name Name of the taskParameter
/// \return taskParameter converted to bool, int, float, double or std::string depending on the template type
template <typename T>
T getFromConfig(const quality_control::core::CustomParameters& params, const std::string_view name, T retVal = T{})
{
  const auto itParam = params.find(name.data());
  if (itParam == params.end()) {
    LOGP(warning, "Missing parameter. Please add '{}': '<value>' to the 'taskParameters'. Using default value {}.", name.data(), retVal);
  } else {
    const auto& param = itParam->second;
    return internal::stringToType<T>(param);
  }
  return retVal;
}

/// \brief Gets a extended task parameter from the config file
/// Convenience function to return a value for a extended taskParameter given in the config file
/// \param activity mActivity from within the QualityControl/TaskInterface
/// \param params mCustomParameters from within the QualityControl/TaskInterface
/// \param name Name of the taskParameter
/// \param retVal Default value if not specified in config, must be stringyfiable
/// \return taskParameter converted to bool, int, float, double or std::string depending on the template type
template <typename T>
T getFromExtendedConfig(const quality_control::core::Activity& activity, const quality_control::core::CustomParameters& params, const std::string& name, T retVal = T{})
{
  std::string parameter;
  if (auto param = params.atOptional(name, activity)) {
    parameter = param.value();
  } else {
    parameter = params.atOrDefaultValue(name, std::to_string(retVal));
  }
  return internal::stringToType<T>(parameter);
}

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_COMMON_UTILS_H
