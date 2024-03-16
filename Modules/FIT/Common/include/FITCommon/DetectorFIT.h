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
/// \file   DetectorFIT.h
/// \author Artur Furs afurs@cern.ch
/// \brief Helper class for FIT detectors

#ifndef QC_MODULE_FIT_DETECTORFIT_H
#define QC_MODULE_FIT_DETECTORFIT_H

#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"

#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

#include "DataFormatsFV0/Digit.h"
#include "DataFormatsFV0/ChannelData.h"

// #include "CommonDataFormat/InteractionRecord.h"

#include <array>
#include <map>
#include <string>
#include <utility>
#include <type_traits>
// #include "FITCommon/HelperFIT.h"
namespace o2::quality_control_modules::fit
{
namespace detectorFIT
{

using TrgMap_t = std::map<unsigned int, std::string>;

enum EDetectorFIT { kFDD,
                    kFT0,
                    kFV0 };
enum ESide { kNothing,
             kSideA,
             kSideC };
#define CREATE_FIT_CHDATA_ACCESSOR(Name, Field)                                                   \
  template <typename T>                                                                           \
  constexpr auto Name(T&& channelData)->std::decay_t<decltype(std::declval<T>().Field)>&&         \
  {                                                                                               \
    return std::forward<std::decay_t<decltype(std::declval<T>().Field)>>(channelData.mChargeADC); \
  }

// FT0 and FV0 has the same field names
CREATE_FIT_CHDATA_ACCESSOR(Amp, mChargeADC)
CREATE_FIT_CHDATA_ACCESSOR(Amp, QTCAmpl)

CREATE_FIT_CHDATA_ACCESSOR(Time, mTime)
CREATE_FIT_CHDATA_ACCESSOR(Time, CFDTime)

CREATE_FIT_CHDATA_ACCESSOR(BitsPM, mFEEBits)
CREATE_FIT_CHDATA_ACCESSOR(BitsPM, ChainQTC)

CREATE_FIT_CHDATA_ACCESSOR(ChannelID, mPMNumber)
CREATE_FIT_CHDATA_ACCESSOR(ChannelID, ChId)

#undef CREATE_FIT_CHDATA_ACCESSOR

template <EDetectorFIT DetID, int NchannelsA, int NchannelsC, int NchannelsNone, typename DigitType, typename ChannelDataType, bool isChIDdirectForSides>
class BaseDetectorFIT
{
 public:
  typedef std::map<unsigned int, std::string> TrgMap_t;

  constexpr static EDetectorFIT sDetFIT_ID = DetID;
  constexpr static int sNchannelsA = NchannelsA;
  constexpr static int sNchannelsC = NchannelsC;
  constexpr static int sNchannelsNone = NchannelsNone;
  constexpr static int sNchannelsAC = sNchannelsA + sNchannelsC;
  constexpr static int sNchannelsAll = sNchannelsAC + sNchannelsNone;
  constexpr static bool sIsChIDdirectForSides = isChIDdirectForSides;
  typedef DigitType Digit_t;
  typedef ChannelDataType ChannelData_t;
  typedef decltype(std::declval<Digit_t>().mTriggers) Triggers_t;
  constexpr static int getSide(int chID)
  {
    if (chID < sNchannelsA) {
      return isChIDdirectForSides ? ESide::kSideA : ESide::kSideC;
    } else if (chID < sNchannelsAC) {
      return isChIDdirectForSides ? ESide::kSideC : ESide::kSideA;
    }
    return ESide::kNothing;
  }
  constexpr static bool isSideA(int side) { return side == ESide::kSideA; }
  constexpr static bool isSideC(int side) { return side == ESide::kSideC; }
  static TrgMap_t addTechTrgBits(const TrgMap_t& trgMap)
  {
    auto newTrgMap = trgMap;
    newTrgMap.insert({ Triggers_t::bitLaser, "Laser" });
    newTrgMap.insert({ Triggers_t::bitOutputsAreBlocked, "OutputsAreBlocked" });
    newTrgMap.insert({ Triggers_t::bitDataIsValid, "DataIsValid" });
    return newTrgMap;
  }
  static const inline TrgMap_t sMapPMbits = {
    { ChannelData_t::kNumberADC, "NumberADC" },
    { ChannelData_t::kIsDoubleEvent, "IsDoubleEvent" },
    { ChannelData_t::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" },
    { ChannelData_t::kIsCFDinADCgate, "IsCFDinADCgate" },
    { ChannelData_t::kIsTimeInfoLate, "IsTimeInfoLate" },
    { ChannelData_t::kIsAmpHigh, "IsAmpHigh" },
    { ChannelData_t::kIsEventInTVDC, "IsEventInTVDC" },
    { ChannelData_t::kIsTimeInfoLost, "IsTimeInfoLost" }
  };

  constexpr static std::array<int, sNchannelsAll> setChIDs2Side()
  {
    std::array<int, sNchannelsAll> chID2side{};
    for (int iCh = 0; iCh < chID2side.size(); iCh++) {
      chID2side[iCh] = getSide(iCh);
    }
    return chID2side;
  }
  constexpr static std::array<int, sNchannelsAll> sArrChID2Side = setChIDs2Side();

  constexpr static std::array<std::vector<uint8_t>, 256> decompose1Byte()
  {
    std::array<std::vector<uint8_t>, 256> arrBitPos{};
    for (int iByteValue = 0; iByteValue < arrBitPos.size(); iByteValue++) {
      auto& vec = arrBitPos[iByteValue];
      for (int iBit = 0; iBit < 8; iBit++) {
        if (iByteValue & (1 << iBit)) {
          vec.push_back(iBit);
        }
      }
    }
    return arrBitPos;
  }
  static const inline std::array<std::vector<uint8_t>, 256> sArrDecomposed1Byte = decompose1Byte();
};

template <EDetectorFIT DetID>
class DetectorFIT : BaseDetectorFIT<DetID, 0, 0, 0, void, void, true>
{
};

template <>
struct DetectorFIT<EDetectorFIT::kFDD> : public BaseDetectorFIT<EDetectorFIT::kFDD, 8, 8, 3, o2::fdd::Digit, o2::fdd::ChannelData, false> {
  static const inline TrgMap_t sMapTrgBits = {
    { Triggers_t::bitA, "OrA" },
    { Triggers_t::bitC, "OrC" },
    { Triggers_t::bitVertex, "Vertex" },
    { Triggers_t::bitCen, "Central" },
    { Triggers_t::bitSCen, "SemiCentral" }
  };
  static const inline TrgMap_t sMapTechTrgBits = addTechTrgBits(sMapTrgBits);
};

template <>
struct DetectorFIT<EDetectorFIT::kFT0> : public BaseDetectorFIT<EDetectorFIT::kFT0, 96, 112, 4, o2::ft0::Digit, o2::ft0::ChannelData, true> {
  static const inline TrgMap_t sMapTrgBits = {
    { Triggers_t::bitA, "OrA" },
    { Triggers_t::bitC, "OrC" },
    { Triggers_t::bitVertex, "Vertex" },
    { Triggers_t::bitCen, "Central" },
    { Triggers_t::bitSCen, "SemiCentral" }
  };
  static const inline TrgMap_t sMapTechTrgBits = addTechTrgBits(sMapTrgBits);
};

template <>
struct DetectorFIT<EDetectorFIT::kFV0> : public BaseDetectorFIT<EDetectorFIT::kFV0, 48, 0, 1, o2::fv0::Digit, o2::fv0::ChannelData, true> {
  static const inline TrgMap_t sMapTrgBits = {
    { o2::fit::Triggers::bitA, "OrA" },
    { o2::fit::Triggers::bitAOut, "OrAOut" },
    { o2::fit::Triggers::bitTrgNchan, "TrgNChan" },
    { o2::fit::Triggers::bitTrgCharge, "TrgCharge" },
    { o2::fit::Triggers::bitAIn, "OrAIn" }

  };
  static const inline TrgMap_t sMapTechTrgBits = addTechTrgBits(sMapTrgBits);
};

using DetectorFDD = DetectorFIT<EDetectorFIT::kFDD>;
using DetectorFT0 = DetectorFIT<EDetectorFIT::kFT0>;
using DetectorFV0 = DetectorFIT<EDetectorFIT::kFV0>;

} // namespace detectorFIT

} // namespace o2::quality_control_modules::fit
#endif
