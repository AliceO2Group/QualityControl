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
/// \author Artur Furs afurs@cern.ch
/// modified by A Khuntia for FDD akhuntia@cern.ch

#ifndef QC_MODULE_FDD_FDDDIGITQCTASKLASER_H
#define QC_MODULE_FDD_FDDDIGITQCTASKLASER_H

#include "CommonConstants/LHCConstants.h"

#include <Framework/InputRecord.h>

#include "QualityControl/QcInfoLogger.h"
#include "FDDBase/Constants.h"
#include "DataFormatsFDD/Digit.h"
#include "DataFormatsFDD/ChannelData.h"
#include "QualityControl/TaskInterface.h"
#include <memory>
#include <regex>
#include <set>
#include <map>
#include <array>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include "TH1.h"
#include "TH2.h"
#include "TList.h"
#include "Rtypes.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::fdd
{

class DigitQcTaskLaser final : public TaskInterface
{
 public:
  /// \brief Constructor
  DigitQcTaskLaser() = default;
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
  constexpr static std::size_t sNCHANNELS_PM = 19;
  constexpr static std::size_t sOrbitsPerTF = 256;
  constexpr static std::size_t sBCperOrbit = o2::constants::lhc::LHCMaxBunches;

 private:
  // three ways of computing cycle duration:
  // 1) number of time frames
  // 2) time in ns from InteractionRecord: total range (totalMax - totalMin)
  // 3) time in ns from InteractionRecord: sum of each TF duration
  // later on choose the best and remove others
  double mTimeMinNS = 0.;
  double mTimeMaxNS = 0.;
  double mTimeCurNS = 0.;
  int mTfCounter = 0;
  double mTimeSum = 0.;

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

  void rebinFromConfig();

  TList* mListHistGarbage;
  std::set<unsigned int> mSetAllowedChIDs;
  std::set<unsigned int> mSetRefPMTChIDs;
  std::array<o2::InteractionRecord, sNCHANNELS_PM> mStateLastIR2Ch;
  std::map<int, std::string> mMapDigitTrgNames;
  std::map<o2::fdd::ChannelData::EEventDataBit, std::string> mMapChTrgNames;
  std::map<std::string, std::vector<int>> mMapPmModuleChannels; // PM name to its channels
  std::unique_ptr<TH1F> mHistNumADC;
  std::unique_ptr<TH1F> mHistNumCFD;

  // Objects which will be published
  std::unique_ptr<TH2F> mHistAmp2Ch;
  std::unique_ptr<TH2F> mHistTime2Ch;
  std::unique_ptr<TH2F> mHistChDataBits;
  std::unique_ptr<TH2F> mHistOrbit2BC;
  std::unique_ptr<TH1F> mHistBC;
  std::unique_ptr<TH1F> mHistCFDEff;
  std::unique_ptr<TH1D> mHistCycleDuration;
  std::map<unsigned int, TH2F*> mMapHistAmpVsBC;
  std::map<std::string, TH2F*> mMapPmModuleBcOrbit;
};

} // namespace o2::quality_control_modules::fdd

#endif // QC_MODULE_FDD_FDDDigitQcTaskLaser_H
