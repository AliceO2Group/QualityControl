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
/// modified by Sebastin Bysiak sbysiak@cern.ch
/// QC Task for FT0 detector, mostly for data visualisation during FEE tests

#ifndef QC_MODULE_FT0_FT0RECOQCTASK_H
#define QC_MODULE_FT0_FT0RECOQCTASK_H

#include <Framework/InputRecord.h>

#include "QualityControl/QcInfoLogger.h"
#include "FT0Base/Constants.h"
#include "DataFormatsFT0/RecPoints.h"
#include "DataFormatsFT0/ChannelData.h"
#include "QualityControl/TaskInterface.h"
#include <memory>
#include <regex>
#include <type_traits>
#include <boost/algorithm/string.hpp>
#include "TH1.h"
#include "TH2.h"
#include "TList.h"
#include "Rtypes.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

class RecPointsQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  RecPointsQcTask() = default;
  /// Destructor
  ~RecPointsQcTask() override;
  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;
  constexpr static std::size_t sOrbitsPerTF = 256;
  constexpr static uint8_t sDataIsValidBitPos = 7;

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
  const float mCFDChannel2NS = 0.01302; // CFD channel width in ns

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
  std::array<o2::InteractionRecord, o2::ft0::Constants::sNCHANNELS_PM> mStateLastIR2Ch;

  // Objects which will be published
  std::unique_ptr<TH2F> mHistAmp2Ch;
  std::unique_ptr<TH2F> mHistTime2Ch;
  std::unique_ptr<TH1F> mHistCollTimeAC;
  std::unique_ptr<TH1F> mHistCollTimeA;
  std::unique_ptr<TH1F> mHistCollTimeC;
  std::unique_ptr<TH1F> mHistResCollTimeA;
  std::unique_ptr<TH1F> mHistResCollTimeC;
  std::unique_ptr<TH2F> mHistTimeSum2Diff;
  std::unique_ptr<TH2F> mHistEventDensity2Ch;
  std::map<unsigned int, TH2F*> mMapHistAmpVsTime;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_FT0RecoQcTask_H
