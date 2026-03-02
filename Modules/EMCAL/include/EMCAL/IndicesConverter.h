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

#ifndef QUALITYCONTROL_INDICESCONVERTER_H
#define QUALITYCONTROL_INDICESCONVERTER_H
#include <map>
#include <string>

namespace o2::emcal
{
class IndicesConverter
{
 public:
  IndicesConverter()
  {
    Initialize();
  }
  ~IndicesConverter() = default;

  void Initialize()
  {
    // Initialize the map with online and offline supermodule indices
    for (int i = 0; i < 20; i++) {
      std::string SMSide = "A";
      if (i % 2 != 0) {
        SMSide = "C";
      }
      int SMRowIndex = (i / 2) + (i >= 12 ? 3 : 0); // Adjust row index for DCAL
      mOnlineToOfflineSMMap[i] = "SM" + SMSide + std::to_string(SMRowIndex);
    }
  }

  int GetOfflineSMIndex(const std::string& onlineSMIndex) const
  {
    for (const auto& pair : mOnlineToOfflineSMMap) {
      if (pair.second == onlineSMIndex) {
        return pair.first;
      }
    }
    return -1; // Return -1 if no matching index is found
  }
  std::string GetOnlineSMIndex(const int offlineSMIndex) const
  {
    if (mOnlineToOfflineSMMap.find(offlineSMIndex) != mOnlineToOfflineSMMap.end()) {
      return mOnlineToOfflineSMMap.at(offlineSMIndex);
    } else {
      return "Invalid SM Index";
    }
  }

 private:
  std::map<int, std::string> mOnlineToOfflineSMMap; ///< Map for conversion between online and offline supermodule indices
};

} // namespace o2::emcal

#endif // QUALITYCONTROL_INDICESCONVERTER_H