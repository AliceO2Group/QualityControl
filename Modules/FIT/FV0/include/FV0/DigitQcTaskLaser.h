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
/// \file   DigitQcTaskLaser.h
/// \author Artur Furs afurs@cern.ch, developed further for laser QC by Sandor Lokos (sandor.lokos@cern.ch)
/// \brief Quality Control DPL Task for FT0's digit visualization for laser events only

#ifndef QC_MODULE_FV0_FV0DIGITQCTASKLASER_H
#define QC_MODULE_FV0_FV0DIGITQCTASKLASER_H

#include <memory>
#include <regex>
#include <type_traits>
#include <set>
#include <map>
#include <vector>
#include <array>
#include <boost/algorithm/string.hpp>

#include "TH1.h"
#include "TH2.h"
#include "TList.h"
#include "Rtypes.h"

#include "CommonConstants/LHCConstants.h"

#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"

#include "FV0Base/Constants.h"
#include "DataFormatsFV0/Digit.h"
#include "DataFormatsFV0/ChannelData.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::fv0
{
class DigitQcTaskLaser final : public TaskInterface
{
 public:
  /// \brief Constructor
  DigitQcTaskLaser() : mHashedBitBinPos(fillHashedBitBinPos()) {}
  /// Destructor
  ~DigitQcTaskLaser() override;
  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  constexpr static std::size_t sNCHANNELS_FV0 = o2::fv0::Constants::nFv0Channels;
  constexpr static std::size_t sNCHANNELS_FV0_PLUSREF = o2::fv0::Constants::nFv0ChannelsPlusRef;
  constexpr static std::size_t sNCHANNELS_FV0_INNER = 24; // "Inner" = 3 inner rings  = first 24 channels
  constexpr static std::size_t sOrbitsPerTF = 256;
  constexpr static std::size_t sBCperOrbit = o2::constants::lhc::LHCMaxBunches;

  constexpr static float sCFDChannel2NS = 0.01302; // CFD channel width in ns

 private:
  template <typename Param_t,
            typename = typename std::enable_if<std::is_floating_point<Param_t>::value ||
                                               std::is_same<std::string, Param_t>::value ||
                                               (std::is_integral<Param_t>::value && !std::is_same<bool, Param_t>::value)>::type>
  auto parseParameters(const std::string& param, const std::string& del)
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

  void rebinFromConfig();
  unsigned int getModeParameter(std::string, unsigned int, std::map<unsigned int, std::string>);
  int getNumericalParameter(std::string, int);

  TList* mListHistGarbage;
  std::set<unsigned int> mSetAllowedChIDs;
  std::set<unsigned int> mSetAllowedChIDsAmpVsTime;
  std::array<o2::InteractionRecord, sNCHANNELS_FV0_PLUSREF> mStateLastIR2Ch;
  std::array<uint8_t, sNCHANNELS_FV0_PLUSREF> mChID2PMhash; // map chID->hashed PM value
  uint8_t mTCMhash;                                         // hash value for TCM, and bin position in hist
  std::map<int, std::string> mMapDigitTrgNames;
  std::map<o2::fv0::ChannelData::EEventDataBit, std::string> mMapChTrgNames;
  std::unique_ptr<TH1F> mHistNumADC;
  std::unique_ptr<TH1F> mHistNumCFD;

  std::map<int, bool> mMapTrgSoftware;
  // only for Inner/Outer trigger
  enum TrgModeThresholdVar { kAmpl,
                             kNchannels
  };
  enum TrgComparisonResult { kSWonly,
                             kTCMonly,
                             kNone,
                             kBoth
  };

  unsigned int mTrgModeInnerOuterThresholdVar;
  // full set of possible parameters:
  // to be eliminated after decision about Inner/Outer trigger type
  int mTrgThresholdCharge;
  int mTrgThresholdChargeOuter;
  int mTrgThresholdChargeInner;
  int mTrgThresholdNChannels;
  int mTrgThresholdNChannelsOuter;
  int mTrgThresholdNChannelsInner;

  // Objects which will be published
  std::unique_ptr<TH2F> mHistAmp2Ch;
  std::unique_ptr<TH2F> mHistTime2Ch;
  std::unique_ptr<TH2F> mHistChDataBits;
  std::unique_ptr<TH1F> mHistBC;
  std::unique_ptr<TH1F> mHistCFDEff;
  //  std::unique_ptr<TH2F> mHistTimeSum2Diff;
  std::map<unsigned int, TH1F*> mMapHistAmp1D;
  std::map<unsigned int, TH1F*> mMapHistTime1D;
  std::map<unsigned int, TH1F*> mMapHistPMbits;
  std::map<unsigned int, TH2F*> mMapHistAmpVsTime;
  std::unique_ptr<TH2F> mHistBCvsFEEmodules;
  std::unique_ptr<TH2F> mHistOrbitVsTrg;
  std::unique_ptr<TH2F> mHistOrbitVsFEEmodules;
  std::unique_ptr<TH1F> mHistTriggersSw;
  std::unique_ptr<TH2F> mHistTriggersSoftwareVsTCM;

  // Hashed maps
  static const size_t mapSize = 256;
  const std::array<std::vector<double>, mapSize> mHashedBitBinPos; // map with bit position for 1 byte trg signal, for 1 Dim hists;
  static std::array<std::vector<double>, mapSize> fillHashedBitBinPos()
  {
    std::array<std::vector<double>, mapSize> hashedBitBinPos{};
    for (int iByteValue = 0; iByteValue < hashedBitBinPos.size(); iByteValue++) {
      auto& vec = hashedBitBinPos[iByteValue];
      for (int iBit = 0; iBit < 8; iBit++) {
        if (iByteValue & (1 << iBit)) {
          vec.push_back(iBit);
        }
      }
    }
    return hashedBitBinPos;
  }
};

} // namespace o2::quality_control_modules::fv0

#endif // QC_MODULE_FV0_FV0DIGITQCTASKLASER_H
