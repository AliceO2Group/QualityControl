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
/// \file   DigitQcTask.h
/// \author Artur Furs afurs@cern.ch
///

#ifndef QC_MODULE_FDD_FDDDIGITQCTASK_H
#define QC_MODULE_FDD_FDDDIGITQCTASK_H

#include <Framework/InputRecord.h>
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"
#include "QualityControl/TaskInterface.h"
#include <memory>
#include <regex>
#include <type_traits>
#include "TH1.h"
#include "TH2.h"
#include "TList.h"
#include "Rtypes.h"
#include "FDD/Helper.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::fdd
{
namespace ch_data = helper::channel_data;

/// \brief Quality Control DPL Task for FDD's digit visualization
/// \author Artur Furs afurs@cern.ch
class DigitQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  DigitQcTask() = default;
  /// Destructor
  ~DigitQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;
  constexpr static std::size_t sNCHANNELS_PM = 28; // 16(for PM) + 12 (just in case for possible PM-LCS)
  constexpr static std::size_t sOrbitsPerTF = 256;
  constexpr static uint8_t sLaserBitPos = 5;

 private:
  template <typename Param_t,
            typename = typename std::enable_if<std::is_floating_point<Param_t>::value ||
                                               std::is_same<std::string, Param_t>::value || (std::is_integral<Param_t>::value && !std::is_same<bool, Param_t>::value)>::type>
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

  // Object which will be published
  std::unique_ptr<TH2F> mHistAmp2Ch;
  std::unique_ptr<TH2F> mHistTime2Ch;
  std::unique_ptr<TH2F> mHistEventDensity2Ch;
  std::unique_ptr<TH2F> mHistOrbit2BC;
  std::unique_ptr<TH2F> mHistChDataBits;
  std::unique_ptr<TH1F> mHistTriggers;
  std::unique_ptr<TH1F> mHistNchA;
  std::unique_ptr<TH1F> mHistNchC;
  std::unique_ptr<TH1F> mHistSumAmpA;
  std::unique_ptr<TH1F> mHistSumAmpC;
  std::unique_ptr<TH1F> mHistAverageTimeA;
  std::unique_ptr<TH1F> mHistAverageTimeC;
  std::unique_ptr<TH1F> mHistChannelID;
  std::array<o2::InteractionRecord, sNCHANNELS_PM> mStateLastIR2Ch;
  std::map<int, std::string> mMapChTrgNames;
  std::map<int, std::string> mMapDigitTrgNames;
  TList* mListHistGarbage;
  std::map<unsigned int, TH1F*> mMapHistAmp1D;
  std::map<unsigned int, TH1F*> mMapHistTime1D;
  std::map<unsigned int, TH1F*> mMapHistPMbits;
  std::map<unsigned int, TH2F*> mMapHistAmpVsTime;
  std::set<unsigned int> mSetAllowedChIDs;
};

} // namespace o2::quality_control_modules::fdd

#endif // QC_MODULE_FDD_FDDDIGITQCTASK_H
