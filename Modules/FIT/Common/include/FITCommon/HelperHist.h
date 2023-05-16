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

#ifndef QC_MODULE_FIT_FITHELPERHIST_H
#define QC_MODULE_FIT_FITHELPERHIST_H

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <memory>
#include <type_traits>

#include <boost/property_tree/ptree.hpp>

#include "TAxis.h"

#include "QualityControl/QcInfoLogger.h"
namespace o2::quality_control_modules::fit
{

namespace helper
{

// Preparation for map axis -> map with bin labels
template <std::size_t NStep, std::size_t NArgLocal, std::size_t NAxis, typename TupleArgs>
inline void getMapBinLabelsPerAxis(std::map<unsigned int, std::map<unsigned int, std::string>>& mapAxisToMapBinLabels, const TupleArgs& tupleArgs)
{
  if constexpr (NStep < std::tuple_size_v<TupleArgs>) {
    using ElementType = typename std::tuple_element<NStep, TupleArgs>::type;
    const auto& arg = std::get<NStep>(tupleArgs);
    constexpr bool isArgMapBinLabels = std::is_same_v<std::decay_t<ElementType>, std::map<unsigned int, std::string>>;
    if constexpr (isArgMapBinLabels && NArgLocal == 0) { // map with bin labels
      mapAxisToMapBinLabels.insert({ NAxis, arg });
      getMapBinLabelsPerAxis<NStep + 1, 0, NAxis + 1>(mapAxisToMapBinLabels, tupleArgs);
    } else if constexpr (!isArgMapBinLabels && NArgLocal < 2) { // first or second bin argument
      getMapBinLabelsPerAxis<NStep + 1, NArgLocal + 1, NAxis>(mapAxisToMapBinLabels, tupleArgs);
    } else if constexpr (!isArgMapBinLabels && NArgLocal == 2) { // third bin argument
      getMapBinLabelsPerAxis<NStep + 1, 0, NAxis + 1>(mapAxisToMapBinLabels, tupleArgs);
    } else {
      // error
      // static_assert();
    }
  }
}

// For unpacking std::map<unsigned int, std::string> into tuple of bin parameters
template <typename Arg>
inline auto unpackHistArg(const Arg& arg)
{
  if constexpr (std::is_same_v<std::decay_t<Arg>, std::map<unsigned int, std::string>>) {
    return std::make_tuple(static_cast<int>(arg.size()), float(0.0), static_cast<float>(arg.size()));
  } else {
    return std::make_tuple(arg);
  }
}

// Makes hist with map bin->label(std::map<unsigned int, std::string>) as bin parameter
// Example:
// auto h1 = makeHist<TH2D>("hist","hist",10,0,10,map1)
// auto h2 = makeHist<TH1F>("hist2","hist2",map1)

template <typename HistType, typename... BinArgs>
inline auto makeHist(const std::string& name, const std::string& title, BinArgs&&... binArgs)
{

  const auto tupleArgs = std::tuple_cat(std::make_tuple(name, title), unpackHistArg(std::forward<BinArgs>(binArgs))...);
  auto ptr = std::apply(
    [](const std::string& name, const std::string& title, auto&&... args) {
      return std::move(std::make_unique<std::decay_t<HistType>>(name.c_str(), title.c_str(), std::forward<decltype(args)>(args)...));
    },
    tupleArgs);

  std::map<unsigned int, std::map<unsigned int, std::string>> mapAxisToMapBinLabels;
  getMapBinLabelsPerAxis<0, 0, 0>(mapAxisToMapBinLabels, std::make_tuple(std::forward<BinArgs>(binArgs)...));
  // Preparing labels
  auto getAxis = [](std::unique_ptr<std::decay_t<HistType>>& ptrHist, unsigned int axis) -> TAxis* {
    if (axis == 0) {
      return ptrHist->GetXaxis();
    } else if (axis == 1) {
      return ptrHist->GetYaxis();
    } else if (axis == 2) {
      return ptrHist->GetZaxis();
    }
    return nullptr;
  };
  auto makeLabels = [](TAxis* axis, const std::map<unsigned int, std::string>& mapBinLabels) {
    for (const auto& entry : mapBinLabels) {
      axis->SetBinLabel(entry.first + 1, entry.second.c_str());
    }
  };
  for (const auto& axisToMapBinLabels : mapAxisToMapBinLabels) {
    makeLabels(getAxis(ptr, axisToMapBinLabels.first), axisToMapBinLabels.second);
  }
  return std::move(ptr);
}

template <typename HistType, typename ManagerType, typename... BinArgs>
inline auto registerHist(ManagerType manager, const std::string& defaultDrawOption, const std::string& name, const std::string& title, BinArgs&&... binArgs)
{
  auto ptrHist = makeHist<HistType>(name, title, std::forward<BinArgs>(binArgs)...);
  manager->startPublishing(ptrHist.get());
  if (defaultDrawOption.size() > 0) {
    manager->setDefaultDrawOptions(ptrHist.get(), defaultDrawOption);
  }
  return std::move(ptrHist);
}

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

inline std::map<unsigned int, std::string> multiplyMaps(const std::vector<std::tuple<std::string, std::map<unsigned int, std::string>, std::string>>& vecPreffixMapSuffix, bool useMapSizeAsMultFactor = true)
{

  auto multiplyPairMaps = [](bool useMapSizeAsMultFactor, const std::tuple<std::string, std::map<unsigned int, std::string>, std::string>& firstPreffixMapSuffix,
                             const std::tuple<std::string, std::map<unsigned int, std::string>, std::string>& secondPreffixMapSuffix = {}) -> std::map<unsigned int, std::string> {
    std::map<unsigned int, std::string> mapResult{};
    const auto& firstPreffix = std::get<0>(firstPreffixMapSuffix);
    const auto& firstSuffix = std::get<2>(firstPreffixMapSuffix);
    const auto& firstMap = std::get<1>(firstPreffixMapSuffix);

    const auto& secondPreffix = std::get<0>(secondPreffixMapSuffix);
    const auto& secondSuffix = std::get<2>(secondPreffixMapSuffix);
    const auto& secondMap = std::get<1>(secondPreffixMapSuffix);

    const unsigned int lastPosSecondMap = secondMap.size() > 0 ? (--secondMap.end())->first : 0;
    const unsigned int multFactor = (useMapSizeAsMultFactor && secondMap.size() > 0) ? secondMap.size() : lastPosSecondMap + 1;

    for (const auto& entryFirst : firstMap) {
      const std::string newEntrySecond_preffix = firstPreffix + entryFirst.second + firstSuffix;
      const unsigned int startIndex = entryFirst.first * multFactor;
      for (const auto& entrySecond : secondMap) {
        const std::string newEntrySecond = newEntrySecond_preffix + secondPreffix + entrySecond.second + secondSuffix;
        const unsigned int newEntryFirst = startIndex + entrySecond.first;
        mapResult.insert({ newEntryFirst, newEntrySecond });
      }
      if (secondMap.size() == 0) {
        // Only add preffix and suffix
        mapResult.insert({ startIndex, newEntrySecond_preffix });
      }
    }
    return mapResult;
  };
  std::map<unsigned int, std::string> mapResult{};
  if (vecPreffixMapSuffix.size() > 0) {
    mapResult = multiplyPairMaps(useMapSizeAsMultFactor, vecPreffixMapSuffix[0]);
    for (int iEntry = 1; iEntry < vecPreffixMapSuffix.size(); iEntry++) {
      const std::string s1{ "" }, s2{ "" };
      mapResult = multiplyPairMaps(useMapSizeAsMultFactor, std::tie(s1, mapResult, s2), vecPreffixMapSuffix[iEntry]);
    }
  }
  return mapResult;
}

} // namespace helper
} // namespace o2::quality_control_modules::fit
#endif