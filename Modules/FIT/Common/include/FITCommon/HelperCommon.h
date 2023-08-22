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
/// \file   HelperHist.h
/// \author Artur Furs afurs@cern.ch
/// \brief Histogram helper

#ifndef QC_MODULE_FIT_HELPERCOMMON_H
#define QC_MODULE_FIT_HELPERCOMMON_H

#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <utility>
#include <memory>
#include <type_traits>
#include <regex>

#include <boost/property_tree/ptree.hpp>

#include "QualityControl/QcInfoLogger.h"
namespace o2::quality_control_modules::fit
{

namespace helper
{

template <typename ValueType>
inline ValueType getConfigFromPropertyTree(const boost::property_tree::ptree& config, const char* fieldName, ValueType value = {})
{
  const auto& node = config.get_child_optional(fieldName);
  if (node) {
    value = node.get_ptr()->get_child("").get_value<ValueType>();
    ILOG(Debug, Support) << fieldName << ": " << value << "\"" << ENDM;
  } else {
    ILOG(Debug, Support) << "Default " << fieldName << ": " << value << ENDM;
  }
  return value;
}

template <typename Param_t,
          typename = typename std::enable_if<std::is_floating_point<Param_t>::value ||
                                             std::is_same<std::string, Param_t>::value || (std::is_integral<Param_t>::value && !std::is_same<bool, Param_t>::value)>::type>
inline auto parseParameters(const std::string& param, const std::string& del)
{
  std::regex reg(del);
  std::sregex_token_iterator first{ param.begin(), param.end(), reg, -1 }, last;
  std::vector<Param_t> vecResult;
  for (auto it = first; it != last; it++) {
    if constexpr (std::is_integral<Param_t>::value && !std::is_same<bool, Param_t>::value) {
      vecResult.push_back(std::stoi(*it));
    } else if constexpr (std::is_floating_point<Param_t>::value) {
      vecResult.push_back(std::stod(*it));
    } else if constexpr (std::is_same<std::string, Param_t>::value) {
      vecResult.push_back(*it);
    }
  }
  return vecResult;
}

// first entry - start BC, second - number of BCs in row
template <typename BitSetBC>
inline std::vector<std::pair<int, int>> getMapBCtrains(const BitSetBC& bitsetBC)
{
  std::vector<std::pair<int, int>> vecTrains{};
  int nBCs{ 0 };
  int firstBC{ -1 };
  for (int iBC = 0; iBC < bitsetBC.size(); iBC++) {
    if (bitsetBC.test(iBC)) {
      // BC in train
      nBCs++;
      if (firstBC == -1) {
        // first BC in train
        firstBC = iBC;
      }
    } else if (nBCs > 0) {
      // Next after end of train BC
      vecTrains.push_back({ firstBC, nBCs });
      firstBC = -1;
      nBCs = 0;
    }
  }
  if (nBCs > 0) { // last iteration
    vecTrains.push_back({ firstBC, nBCs });
  }
  return vecTrains;
}

std::map<unsigned int, std::string> multiplyMaps(const std::vector<std::tuple<std::string, std::map<unsigned int, std::string>, std::string>>& vecPreffixMapSuffix, bool useMapSizeAsMultFactor = true);

template <typename R, typename... Args, std::size_t... IArgs>
auto funcWithArgsAsTupleBase(std::function<R(Args...)> func, const std::tuple<Args...>& tupleArgs, std::index_sequence<IArgs...>)
{
  return func(std::get<IArgs>(tupleArgs)...);
}

template <typename R, typename... Args>
auto funcWithArgsAsTuple(std::function<R(Args...)> func, const std::tuple<Args...>& tupleArgs)
{
  return funcWithArgsAsTupleBase(func, tupleArgs, std::make_index_sequence<sizeof...(Args)>{});
}

} // namespace helper
} // namespace o2::quality_control_modules::fit
#endif