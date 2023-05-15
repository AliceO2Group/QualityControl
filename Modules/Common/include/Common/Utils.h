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

/// \brief Gets a task parameter from the config file
/// Convenience function to return a value for a taskParameter given in the config file
/// \param params mCustomParameters from within the QualityControl/TaskInterface
/// \param name Name of the taskParameter
/// \return taskParameter converted to bool, int, float, double or std::string depending on the template type
template <typename T>
T getFromConfig(const quality_control::core::CustomParameters& params, const std::string_view name, T retVal = T{})
{
  const auto last = params.end();
  const auto itParam = params.find(name.data());

  if (itParam == last) {
    LOGP(warning, "missing parameter {}", name.data());
    LOGP(warning, "Please add '{}': '<value>' to the 'taskParameters'. Using default value {}.", name.data(), retVal);
  } else {
    const auto& param = itParam->second;
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
        LOG(error) << fmt::format("Please provide a valid boolean value for {}.", name.data()).data();
      }
    } else if constexpr (std::is_same<std::string, T>::value) {
      retVal = param;
    } else {
      LOG(error) << "Template type not supported";
    }
  }
  return retVal;
}

} // namespace o2::quality_control_modules::common

#endif // QC_MODULE_COMMON_UTILS_H
