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
/// \file   HelperHist.cxx
/// \author Artur Furs afurs@cern.ch

#include "FITCommon/HelperHist.h"

namespace o2::quality_control_modules::fit
{
namespace helper
{

std::map<unsigned int, std::string> multiplyMaps(const std::vector<std::tuple<std::string, std::map<unsigned int, std::string>, std::string>>& vecPreffixMapSuffix, bool useMapSizeAsMultFactor = true)
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