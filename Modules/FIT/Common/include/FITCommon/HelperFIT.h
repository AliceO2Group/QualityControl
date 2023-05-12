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
/// \brief Helper class for FIT detectors

#ifndef QC_MODULE_FIT_FITHELPER_H
#define QC_MODULE_FIT_FITHELPER_H

#include <map>
#include <string>
#include <utility>
#include <unordered_map>
#include <functional>
#include <array>
#include <vector>
#include <DataFormatsFIT/Triggers.h>

namespace o2::quality_control_modules::fit
{

template <typename DigitType, typename ChannelDataType>
class HelperFIT
{
 public:
  using ChannelData_t = ChannelDataType;
  using Digit_t = DigitType;
  typedef decltype(std::declval<Digit_t>().mTriggers) Triggers_t;

  const std::map<unsigned int, std::string> mMapPMbits = {
    { ChannelData_t::kNumberADC, "NumberADC" },
    { ChannelData_t::kIsDoubleEvent, "IsDoubleEvent" },
    { ChannelData_t::kIsTimeInfoNOTvalid, "IsTimeInfoNOTvalid" },
    { ChannelData_t::kIsCFDinADCgate, "IsCFDinADCgate" },
    { ChannelData_t::kIsTimeInfoLate, "IsTimeInfoLate" },
    { ChannelData_t::kIsAmpHigh, "IsAmpHigh" },
    { ChannelData_t::kIsEventInTVDC, "IsEventInTVDC" },
    { ChannelData_t::kIsTimeInfoLost, "IsTimeInfoLost" }
  };
};

class HelperTrgFIT
{
 public:
  HelperTrgFIT() = delete;
  ~HelperTrgFIT() = delete;
  static const std::map<unsigned int, std::string> sMapTrgBits;
  static const std::map<unsigned int, std::string> sMapBasicTrgBitsFDD;
  static const std::map<unsigned int, std::string> sMapBasicTrgBitsFT0;
  static const std::map<unsigned int, std::string> sMapBasicTrgBitsFV0;
  static const std::array<std::vector<uint8_t>, 256> sArrDecomposed1Byte;
  inline static std::array<std::vector<uint8_t>, 256> decompose1Byte()
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
};

template <typename DigitType>
struct DataTCM {
  using Digit_t = DigitType;
  typedef decltype(std::declval<Digit_t>().mTriggers) Triggers_t;
  // Copied from o2::fit::Triggers
  //  will be moved there soon
  uint8_t triggersignals{}; // FIT trigger signals
  uint8_t nChanA{};         // number of fired channels A side
  uint8_t nChanC{};         // number of fired channels A side
  int32_t amplA{};          // sum amplitude A side
  int32_t amplC{};          // sum amplitude C side
  int16_t timeA{};          // average time A side (shouldn't be used if nChanA == 0)
  int16_t timeC{};          // average time C side (shouldn't be used if nChanC == 0)

  int32_t amplSum{};
  uint8_t nChanSum{};
  template <typename AmpType, typename TimeType>
  void fillSideA(const AmpType& amp, const TimeType& time)
  {
    amplA += amp;
    timeA += time;
    nChanA++;
  }
  template <typename AmpType, typename TimeType>
  void fillSideC(const AmpType& amp, const TimeType& time)
  {
    amplC += amp;
    timeC += time;
    nChanA++;
  }
  void finalize()
  {
    amplA = amplA / 8;
    amplC = amplC / 8;
    amplSum = amplA + amplC;
    nChanSum = nChanA + nChanC;
    if (nChanA > 0) {
      timeA = timeA / nChanA;
      triggersignals |= (1 << Triggers_t::bitA);
    }
    if (nChanC > 0) {
      timeC = timeC / nChanC;
      triggersignals |= (1 << Triggers_t::bitC);
    }
  }
};
template <typename DigitType, typename ChannelDataType>
class TrgValidation
{
 public:
  using ChannelData_t = ChannelDataType;
  using Digit_t = DigitType;
  typedef decltype(std::declval<Digit_t>().mTriggers) Triggers_t;
  using DataTCM_t = DataTCM<Digit_t>;
  void configure(const std::unordered_map<std::string, std::string>& params)
  {
  }

  // - time window for vertex trigger
  int mTrgThresholdTimeLow;
  int mTrgThresholdTimeHigh;
  // - parameters for (Semi)Central triggers
  //   same parameters re-used for both Ampl and Nchannels thresholds
  int mTrgThresholdCenA;
  int mTrgThresholdCenC;
  int mTrgThresholdSCenA;
  int mTrgThresholdSCenC;
  int mTrgChargeLevelLow;
  int mTrgChargeLevelHigh;
  int mTrgOrGate;

  enum ETriggerValidation { // first bit - HW, second - SW
    kBothOff = 0b00,
    kOnlyHW = 0b01,
    kOnlySW = 0b10,
    kBothOn = 0b11
  };
  constexpr static uint8_t sPMbitsGood = (1 << ChannelData_t::kIsCFDinADCgate);
  constexpr static uint8_t sPMbitsBad = (1 << ChannelData_t::kIsTimeInfoNOTvalid) |
                                        (1 << ChannelData_t::kIsTimeInfoLate) |
                                        (1 << ChannelData_t::kIsAmpHigh) |
                                        (1 << ChannelData_t::kIsTimeInfoLost);
  constexpr static uint8_t sPMbitsToCheck = sPMbitsGood | sPMbitsBad;
  const std::map<unsigned int, std::string> mMapTrgValidation = {
    { ETriggerValidation::kBothOff, "Both off" },
    { ETriggerValidation::kOnlyHW, "Only HW" },
    { ETriggerValidation::kOnlySW, "Only SW" },
    { ETriggerValidation::kBothOn, "Both on" }
  };
  // To get trigger validation status
  static ETriggerValidation getTrgValidationStatus(uint8_t hwTrg, uint8_t swTrg, uint8_t trgBitPos)
  {
    uint8_t status = (((hwTrg >> trgBitPos) & 1) | (((swTrg << 1) >> trgBitPos) & 0b10));
    return static_cast<ETriggerValidation>(status);
  };
  const std::map<std::pair<std::string, std::string>, std::function<void(DataTCM_t&)>> mMapTrgCalcFunctors = {
    { { "Ampl", "A+C" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.amplSum >= mTrgThresholdCenA) << Triggers_t::bitCen);
       const uint8_t trg = ((tcm.amplSum >= mTrgThresholdSCenA && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } },
    { { "Ampl", "A&C" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.amplA >= mTrgThresholdCenA && tcm.amplC >= mTrgThresholdCenC) << Triggers_t::bitCen);
       const uint8_t trg = (((tcm.amplA >= mTrgThresholdSCenA && tcm.amplC >= mTrgThresholdSCenC) && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } },
    { { "Ampl", "A" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.amplA >= mTrgThresholdCenA) << Triggers_t::bitCen);
       const uint8_t trg = ((tcm.amplA >= mTrgThresholdSCenA && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } },
    { { "Ampl", "C" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.amplC >= mTrgThresholdCenC) << Triggers_t::bitCen);
       const uint8_t trg = ((tcm.amplC >= mTrgThresholdSCenC && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } },
    { { "Nchannels", "A+C" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.nChanSum >= mTrgThresholdCenA) << Triggers_t::bitCen);
       const uint8_t trg = ((tcm.nChanSum >= mTrgThresholdSCenA && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } },
    { { "Nchannels", "A&C" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.nChanA >= mTrgThresholdCenA && tcm.nChanC >= mTrgThresholdCenC) << Triggers_t::bitCen);
       const uint8_t trg = (((tcm.nChanA >= mTrgThresholdSCenA && tcm.nChanC >= mTrgThresholdSCenC) && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } },
    { { "Nchannels", "A" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.nChanA >= mTrgThresholdCenA) << Triggers_t::bitCen);
       const uint8_t trg = ((tcm.nChanA >= mTrgThresholdSCenA && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } },
    { { "Nchannels", "C" }, [this](DataTCM_t& tcm) {
       const uint8_t cent = ((tcm.nChanC >= mTrgThresholdCenC) << Triggers_t::bitCen);
       const uint8_t trg = ((tcm.nChanC >= mTrgThresholdSCenC && cent == 0) << Triggers_t::bitSCen) | cent;
       tcm.triggersignals |= trg;
     } }
  };
  std::function<void(DataTCM_t&)> mFunctorTrgCalc;
};

} // namespace o2::quality_control_modules::fit
#endif
