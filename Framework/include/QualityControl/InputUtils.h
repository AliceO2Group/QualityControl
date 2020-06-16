// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QC_INPUT_UTILS_H
#define QC_INPUT_UTILS_H

// std
#include <utility>
#include <vector>
#include <string>
//o2
#include <Framework/DataProcessorSpec.h>
#include <Framework/DataSpecUtils.h>

inline std::vector<std::string> stringifyInput(o2::framework::Inputs& inputs)
{
  std::vector<std::string> vec;
  for (auto& input : inputs) {
    vec.push_back(o2::framework::DataSpecUtils::describe(input));
  }
  return vec;
}

#endif
