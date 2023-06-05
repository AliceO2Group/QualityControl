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
/// \file   DigitSync.h
/// \author Artur Furs afurs@cern.ch
/// \brief Class for synchronization of FIT detector digits

#ifndef QC_MODULE_FIT_DIGITSYNC_H
#define QC_MODULE_FIT_DIGITSYNC_H

#include <map>
#include <string>
#include <utility>
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <bitset>
#include <array>

#include "CommonDataFormat/InteractionRecord.h"

namespace o2::quality_control_modules::fit
{

template <typename DigitFDDtype, typename DigitFT0type, typename DigitFV0type>
struct DigitSync {
 public:
  typedef DigitFDDtype DigitFDD;
  typedef DigitFT0type DigitFT0;
  typedef DigitFV0type DigitFV0;
  using Index_t = uint32_t;
  typedef std::map<o2::InteractionRecord, DigitSync> MapIR2Digits_t;
  enum EDetectorBit { kFDD,
                      kFT0,
                      kFV0
  };
  constexpr static std::size_t sNdetectors = 3;
  std::bitset<sNdetectors> mActiveDets;
  std::array<Index_t, sNdetectors> mDigitIndexes{};
  template <typename DigitType>
  static auto getIR(DigitType&& digit) -> std::enable_if_t<std::is_member_function_pointer_v<decltype(&std::decay_t<decltype(digit)>::getInteractionRecord)>, o2::InteractionRecord>
  {
    return (std::forward<DigitType>(digit)).getInteractionRecord();
  }

  template <typename DigitType>
  static auto getIR(DigitType&& digit) -> std::enable_if_t<std::is_member_function_pointer_v<decltype(&std::decay_t<decltype(digit)>::getIntRecord)>, o2::InteractionRecord>
  {
    return (std::forward<DigitType>(digit)).getIntRecord();
  }

  template <typename ContType>
  static void fillSyncMap(MapIR2Digits_t& mapIR2Digits, const ContType& vecDigits)
  {
    uint32_t index{ 0 };
    using DigitType = typename ContType::value_type; // span or vector
    for (const auto& digit : vecDigits) {
      const auto ir = getIR(digit);
      auto pairInserted = mapIR2Digits.insert({ ir, {} });
      if constexpr (std::is_same<DigitFDD, DigitType>::value) {
        pairInserted.first->second.mActiveDets.set(EDetectorBit::kFDD);
        pairInserted.first->second.mDigitIndexes[EDetectorBit::kFDD] = index;
      } else if (std::is_same<DigitFT0, DigitType>::value) {
        pairInserted.first->second.mActiveDets.set(EDetectorBit::kFT0);
        pairInserted.first->second.mDigitIndexes[EDetectorBit::kFT0] = index;
      } else if (std::is_same<DigitFV0, DigitType>::value) {
        pairInserted.first->second.mActiveDets.set(EDetectorBit::kFV0);
        pairInserted.first->second.mDigitIndexes[EDetectorBit::kFV0] = index;
      } else {
        constexpr bool breakCompilation = true;
        static_assert(breakCompilation == true, "Unrecognized digit type!");
      }
      index++;
    }
  }
  template <typename DigitFDDcont, typename DigitFT0cont, typename DigitFV0cont>
  static MapIR2Digits_t makeSyncMap(const DigitFDDcont& vecDigitsFDD,
                                    const DigitFT0cont& vecDigitsFT0,
                                    const DigitFV0cont& vecDigitsFV0)
  {
    MapIR2Digits_t mapIR2Digits{};
    fillSyncMap(mapIR2Digits, vecDigitsFDD);
    fillSyncMap(mapIR2Digits, vecDigitsFT0);
    fillSyncMap(mapIR2Digits, vecDigitsFV0);
    return mapIR2Digits;
  }
  Index_t getIndexFDD() const { return mDigitIndexes[EDetectorBit::kFDD]; }
  Index_t getIndexFT0() const { return mDigitIndexes[EDetectorBit::kFT0]; }
  Index_t getIndexFV0() const { return mDigitIndexes[EDetectorBit::kFV0]; }

  bool isFDD() const { return mActiveDets.test(EDetectorBit::kFDD); }
  bool isFT0() const { return mActiveDets.test(EDetectorBit::kFT0); }
  bool isFV0() const { return mActiveDets.test(EDetectorBit::kFV0); }

  template <typename VecDigitsFDDtype>
  const DigitFDD& getDigitFDD(const VecDigitsFDDtype& vecDigitsFDD) const
  {
    return isFDD() ? vecDigitsFDD[getIndexFDD()] : sDummyDigitFDD;
  }
  template <typename VecDigitsFT0type>
  const DigitFT0& getDigitFT0(const VecDigitsFT0type& vecDigitsFT0) const
  {
    return isFT0() ? vecDigitsFT0[getIndexFT0()] : sDummyDigitFT0;
  }
  template <typename VecDigitsFV0type>
  const DigitFV0& getDigitFV0(const VecDigitsFV0type& vecDigitsFV0) const
  {
    return isFV0() ? vecDigitsFV0[getIndexFV0()] : sDummyDigitFV0;
  }

 private:
  static const DigitFDD sDummyDigitFDD;
  static const DigitFT0 sDummyDigitFT0;
  static const DigitFV0 sDummyDigitFV0;
};
template <typename DigitFDDtype, typename DigitFT0type, typename DigitFV0type>
const typename DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::DigitFDD
  DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::sDummyDigitFDD = typename DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::DigitFDD{};

template <typename DigitFDDtype, typename DigitFT0type, typename DigitFV0type>
const typename DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::DigitFT0
  DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::sDummyDigitFT0 = typename DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::DigitFT0{};

template <typename DigitFDDtype, typename DigitFT0type, typename DigitFV0type>
const typename DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::DigitFV0
  DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::sDummyDigitFV0 = typename DigitSync<DigitFDDtype, DigitFT0type, DigitFV0type>::DigitFV0{};

} // namespace o2::quality_control_modules::fit
#endif
