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
#include <vector>

namespace o2::quality_control::core
{
class CustomParameters;

std::vector<std::string> getBinRepresentation(unsigned char* data, size_t size);
std::vector<std::string> getHexRepresentation(unsigned char* data, size_t size);

/// \brief Decode key of a configurable parameter as boolean
/// \param value Value to be decoded (true or false, case-insensitive)
/// \return Boolean representation of the value
/// \throw std::runtime_error in case value is not a boolean value
bool decodeBool(const std::string& value);

/// Utility methods to fetch boolean options from the custom parameters.
/// @param name name of the option as in the mCustomParameters and JSON file
/// @return true if the option was found in the config and it was set to true, false if it was set to false
/// @throw AliceO2::Common::ObjectNotFoundError if 'name' is not found in mCustomParameters
/// @throw std::runtime_error the value is not a bool
bool parseBoolParam(const CustomParameters& customParameters, const std::string& name, const std::string& runType = "default", const std::string& beamType = "default");

} // namespace o2::quality_control::core

#endif // QC_STRING_UTILS_H
