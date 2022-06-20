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
/// \file   Utils.cxx
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Set of common utilities for Tasks and Checkers
///

namespace o2::quality_control_modules::tof::utils
{

/// Utility methods to fetch float options from the custom parameters.
/// @param parametersIn container for the input parameters
/// @param name name of the option as in the mCustomParameters and JSON file
/// @param parameterOut will be set accordingly if the 'name' element is in mCustomParameters
/// @return true if the option was found, false otherwise
template <typename ParameterType>
bool parseBooleanParameter(const ParameterType& parametersIn, const std::string& name, bool& parameterOut)
{
  if (auto param = parametersIn.find(name); param != parametersIn.end()) {
    ILOG(Info, Devel) << "Custom parameter - " << name << " " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      parameterOut = true;
    } else if (param->second == "false" || param->second == "False" || param->second == "FALSE") {
      parameterOut = false;
    }
    return true;
  }
  return false;
}

/// Utility methods to fetch float options from the custom parameters.
/// @param parametersIn container for the input parameters
/// @param name name of the option as in the mCustomParameters and JSON file
/// @param parameterOut will be set accordingly if the 'name' element is in mCustomParameters
/// @return true if the option was found, false otherwise
template <typename ParameterType>
bool parseDoubleParameter(const ParameterType& parametersIn, const std::string& name, double& parameterOut)
{
  if (auto param = parametersIn.find(name); param != parametersIn.end()) {
    ILOG(Info, Devel) << "Custom parameter - " << name << " " << param->second << ENDM;
    parameterOut = ::atof(param->second.c_str());
    return true;
  }
  return false;
}

/// Utility methods to fetch float options from the custom parameters.
/// @param parametersIn container for the input parameters
/// @param name name of the option as in the mCustomParameters and JSON file
/// @param parameterOut will be set accordingly if the 'name' element is in mCustomParameters
/// @return true if the option was found, false otherwise
template <typename ParameterType>
bool parseFloatParameter(const ParameterType& parametersIn, const std::string& name, float& parameterOut)
{
  if (auto param = parametersIn.find(name); param != parametersIn.end()) {
    ILOG(Info, Devel) << "Custom parameter - " << name << " " << param->second << ENDM;
    parameterOut = ::atof(param->second.c_str());
    return true;
  }
  return false;
}

/// Utility methods to fetch float options from the custom parameters.
/// @param parametersIn container for the input parameters
/// @param name name of the option as in the mCustomParameters and JSON file
/// @param parameter will be set accordingly if the 'name' element is in mCustomParameters
/// @return true if the option was found, false otherwise
template <typename ParameterType>
bool parseIntParameter(const ParameterType& parametersIn, const std::string& name, int& parameterOut)
{
  if (auto param = parametersIn.find(name); param != parametersIn.end()) {
    ILOG(Info, Devel) << "Custom parameter - " << name << " " << param->second << ENDM;
    parameterOut = ::atoi(param->second.c_str());
    return true;
  }
  return false;
}

/// Utility methods to fetch float options from the custom parameters.
/// @param parametersIn container for the input parameters
/// @param name name of the option as in the mCustomParameters and JSON file
/// @param parameter will be set accordingly if the 'name' element is in mCustomParameters
/// @return true if the option was found, false otherwise
template <typename ParameterType>
bool parseStrParameter(const ParameterType& parametersIn, const std::string& name, std::string& parameterOut)
{
  if (auto param = parametersIn.find(name); param != parametersIn.end()) {
    ILOG(Info, Devel) << "Custom parameter - " << name << " " << param->second << ENDM;
    parameterOut = param->second;
    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::tof::utils