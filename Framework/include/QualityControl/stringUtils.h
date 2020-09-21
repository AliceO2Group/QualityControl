// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
} // namespace o2::quality_control::core

#endif // QC_STRING_UTILS_H
