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
#include <set>
#include <string>
#include <utility>
#include <unordered_map>
#include <functional>
#include <array>
#include <vector>
#include <bitset>

#include "TH1D.h"
#include "TH2F.h"

#include <DataFormatsFIT/Triggers.h>
#include "CommonDataFormat/BunchFilling.h"
#include "QualityControl/CustomParameters.h"
#include "Common/Utils.h"

#include <FITCommon/HelperHist.h>
namespace o2::quality_control_modules::fit
{

template <typename DigitType>
struct DataTCM {
  using Digit_t = DigitType;
  typedef decltype(std::declval<Digit_t>().mTriggers) Triggers_t;
  // Copied from o2::fit::Triggers
  //  will be moved there soon
  DataTCM(int32_t _amplA, int32_t _amplC, int sumTimeA, int sumTimeC, uint8_t _nChanA, uint8_t _nChanC, uint8_t _triggersignals = 0) : triggersignals(_triggersignals), nChanA(_nChanA), nChanC(_nChanC), amplA(_amplA), amplC(_amplC), timeA(divHW_TCM(sumTimeA, _nChanA)), timeC(divHW_TCM(sumTimeC, _nChanC))
  {
  }

  uint8_t triggersignals{}; // FIT trigger signals
  uint8_t nChanA{};         // number of fired channels A side
  uint8_t nChanC{};         // number of fired channels A side
  int32_t amplA{};          // sum amplitude A side
  int32_t amplC{};          // sum amplitude C side
  int16_t timeA{};          // average time A side (shouldn't be used if nChanA == 0)
  int16_t timeC{};          // average time C side (shouldn't be used if nChanC == 0)

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

  static int divHW_TCM(int sumTime, int nChannels)
  {
    constexpr std::array<int, 127> rom7x17{ 16383, 8192, 5461, 4096, 3277, 2731, 2341, 2048, 1820, 1638, 1489, 1365, 1260, 1170, 1092, 1024, 964, 910, 862, 819, 780, 745, 712, 683, 655, 630, 607, 585, 565, 546, 529, 512, 496, 482, 468, 455, 443, 431, 420, 410, 400, 390, 381, 372, 364, 356, 349, 341, 334, 328, 321, 315, 309, 303, 298, 293, 287, 282, 278, 273, 269, 264, 260, 256, 252, 248, 245, 241, 237, 234, 231, 228, 224, 221, 218, 216, 213, 210, 207, 205, 202, 200, 197, 195, 193, 191, 188, 186, 184, 182, 180, 178, 176, 174, 172, 171, 169, 167, 165, 164, 162, 161, 159, 158, 156, 155, 153, 152, 150, 149, 148, 146, 145, 144, 142, 141, 140, 139, 138, 137, 135, 134, 133, 132, 131, 130, 129 };
    return nChannels ? (sumTime * rom7x17[nChannels - 1]) >> 14 : 0;
  }
};
template <typename DigitType>
class TrgValidation
{
 public:
  using Digit_t = DigitType;
  typedef decltype(std::declval<Digit_t>().mTriggers) Triggers_t;
  using DataTCM_t = DataTCM<Digit_t>;
  void configure(const o2::quality_control::core::CustomParameters& customParameters)
  {
    const std::string trgModeThresholdVar = o2::quality_control_modules::common::getFromConfig<std::string>(customParameters, "trgModeThresholdVar", "Ampl");
    const std::string trgModeSide = o2::quality_control_modules::common::getFromConfig<std::string>(customParameters, "trgModeSide", "A+C");
    const auto itFunc = mMapTrgCalcFunctors.find({ trgModeThresholdVar, trgModeSide });
    assert(itFunc != mMapTrgCalcFunctors.end());
    mFunctorTrgCalc = itFunc->second;

    mTrgOrGate = o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgOrGate", 153);
    mTrgChargeLevelLow = o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgChargeLevelLow", 0);
    mTrgChargeLevelHigh = o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgChargeLevelHigh", 4095);
    mTrgThresholdTimeLow = o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgThresholdTimeLow", -192);
    mTrgThresholdTimeHigh = o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgThresholdTimeHigh", 192);

    int thrshFactor = (trgModeThresholdVar == std::string{ "Ampl" }) ? 2 : 1; // 2 for Ampl, 1 for Nchannels. TODO: better to use mapping
    mTrgThresholdCenA = thrshFactor * o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgThresholdCenA", 20);
    mTrgThresholdCenC = thrshFactor * o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgThresholdCenC", 20);
    mTrgThresholdSCenA = thrshFactor * o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgThresholdSCenA", 10);
    mTrgThresholdSCenC = thrshFactor * o2::quality_control_modules::common::getFromConfig<int>(customParameters, "trgThresholdSCenC", 10);
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

  static const inline std::map<unsigned int, std::string> sMapTrgValidation = {
    { ETriggerValidation::kBothOff, "Both off" },
    { ETriggerValidation::kOnlyHW, "Only HW" },
    { ETriggerValidation::kOnlySW, "Only SW" },
    { ETriggerValidation::kBothOn, "Both on" }
  };
  // To get trigger validation status
  static ETriggerValidation getTrgValidationStatus(uint8_t hwTrg, uint8_t swTrg, uint8_t trgBitPos)
  {
    // |SW bit|HW bit|
    uint8_t status = (((hwTrg >> trgBitPos) & 1) | (((swTrg << 1) >> trgBitPos) & 0b10));
    return static_cast<ETriggerValidation>(status);
  };

  void emulateTimeTriggers(DataTCM_t& tcm)
  {
    const bool isOrA = tcm.nChanA > 0;
    const bool isOrC = tcm.nChanC > 0;
    tcm.triggersignals |= (isOrA << Triggers_t::bitA);
    tcm.triggersignals |= (isOrC << Triggers_t::bitC);
    const int meanTimeDiff = tcm.timeC - tcm.timeA;
    const bool isVertexEmu = mTrgThresholdTimeLow < meanTimeDiff && meanTimeDiff < mTrgThresholdTimeHigh && isOrA && isOrC;
    tcm.triggersignals |= (isVertexEmu << Triggers_t::bitVertex);
  }
  void emulateTriggers(DataTCM_t& tcm)
  {
    emulateTimeTriggers(tcm); // Time trigger emulation
    mFunctorTrgCalc(tcm);     // Amp and channel triggers
  }
  const std::map<std::pair<std::string, std::string>, std::function<void(DataTCM_t&)>> mMapTrgCalcFunctors = {
    { { "Ampl", "A+C" }, [this](DataTCM_t& tcm) {
       const auto sumAmp = tcm.amplA + tcm.amplC;
       const bool cent = sumAmp > mTrgThresholdCenA;
       const bool scent = cent == false && sumAmp > mTrgThresholdSCenA;
       // std::cout<<std::endl<<"CFG: "<< mTrgThresholdCenA<<"|"<<mTrgThresholdSCenA;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } },
    { { "Ampl", "A&C" }, [this](DataTCM_t& tcm) {
       const bool cent = tcm.amplA > mTrgThresholdCenA && tcm.amplC > mTrgThresholdCenC;
       const bool scent = cent == false && tcm.amplA > mTrgThresholdSCenA && tcm.amplC > mTrgThresholdSCenC;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } },
    { { "Ampl", "A" }, [this](DataTCM_t& tcm) {
       const bool cent = tcm.amplA > mTrgThresholdCenA;
       const bool scent = cent == false && tcm.amplA > mTrgThresholdSCenA;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } },
    { { "Ampl", "C" }, [this](DataTCM_t& tcm) {
       const bool cent = tcm.amplC > mTrgThresholdCenC;
       const bool scent = cent == false && tcm.amplC > mTrgThresholdSCenC;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } },
    { { "Nchannels", "A+C" }, [this](DataTCM_t& tcm) {
       const auto nChannels = tcm.nChanA + tcm.nChanC;
       const bool cent = nChannels > mTrgThresholdCenA;
       const bool scent = cent == false && nChannels > mTrgThresholdSCenA;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } },
    { { "Nchannels", "A&C" }, [this](DataTCM_t& tcm) {
       const bool cent = tcm.nChanA > mTrgThresholdCenA && tcm.nChanC > mTrgThresholdCenC;
       const bool scent = cent == false && tcm.nChanA > mTrgThresholdSCenA && tcm.nChanC > mTrgThresholdSCenC;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } },
    { { "Nchannels", "A" }, [this](DataTCM_t& tcm) {
       const bool cent = tcm.nChanA > mTrgThresholdCenA;
       const bool scent = cent == false && tcm.nChanA > mTrgThresholdSCenA;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } },
    { { "Nchannels", "C" }, [this](DataTCM_t& tcm) {
       const bool cent = tcm.nChanC > mTrgThresholdCenC;
       const bool scent = cent == false && tcm.nChanC > mTrgThresholdSCenC;
       tcm.triggersignals |= ((cent << Triggers_t::bitCen) | (scent << Triggers_t::bitSCen));
     } }
  };
  std::function<void(DataTCM_t&)> mFunctorTrgCalc;
};

} // namespace o2::quality_control_modules::fit
#endif
