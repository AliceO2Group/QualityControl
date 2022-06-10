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
/// \file   stringUtils.h
/// \author Barthelemy von Haller
///

#ifndef QC_STRING_UTILS_H
#define QC_STRING_UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <bitset>

namespace o2::quality_control::core
{

std::vector<std::string> getBinRepresentation(unsigned char* data, size_t size)
{
  std::stringstream ss;
  std::vector<std::string> result;
  result.reserve(size);

  for (size_t i = 0; i < size; i++) {
    std::bitset<16> x(data[i]);
    ss << x << " ";
    result.push_back(ss.str());
    ss.str(std::string());
  }
  return result;
}

std::vector<std::string> getHexRepresentation(unsigned char* data, size_t size)
{
  std::stringstream ss;
  std::vector<std::string> result;
  result.reserve(size);
  ss << std::hex << std::setfill('0');

  for (size_t i = 0; i < size; i++) {
    ss << std::setw(2) << static_cast<unsigned>(data[i]) << " ";
    result.push_back(ss.str());
    ss.str(std::string());
  }
  return result;
}

/// Utility methods to fetch boolean otpions from the custom parameters.
/// @param name name of the option as in the mCustomParameters and JSON file
/// @param flag will be set accordingly if the 'name' element is in mCustomParameters
/// @return true if the option was found, false otherwise
bool parseBooleanParam(std::unordered_map<std::string, std::string> customParameters, const std::string& name,
                           bool& flag)
{
  if (auto param = customParameters.find(name); param != customParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - " << name << " " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE" || param->second == "1") {
      flag = true;
    } else if (param->second == "false" || param->second == "False" || param->second == "FALSE" || param->second == "0") {
      flag = false;
    }
    return true;
  }
  return false;
}

} // namespace o2::quality_control::core

#endif // QC_STRING_UTILS_H
