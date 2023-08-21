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
/// \file   DigitsCheck.h
/// \author Andrea Ferrero, Sebastien Perrin
///

#ifndef QC_MODULE_MCH_DIGITSCHECK_H
#define QC_MODULE_MCH_DIGITSCHECK_H

#include "MCH/Helpers.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/Quality.h"
#include "MCHRawElecMap/Mapper.h"
#include <string>

using namespace o2::quality_control_modules::muon;

namespace o2::quality_control::core
{
class MonitorObject;
}

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check if the occupancy on each pad is between the two specified values
///
/// \author Andrea Ferrero, Sebastien Perrin
class DigitsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  DigitsCheck() = default;
  /// Destructor
  ~DigitsCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  std::array<Quality, getNumDE()> checkMeanRates(TH1F* h);
  std::array<Quality, getNumDE()> checkMeanRatesRatio(TH1F* h);
  std::array<Quality, getNumDE()> checkBadChannels(TH1F* h);
  std::array<Quality, getNumDE()> checkBadChannelsRatio(TH1F* h);

  std::string mMeanRateHistName{ "RatesSignal/LastCycle/MeanRate" };
  std::string mMeanRateRatioHistName{ "RatesSignal/LastCycle/MeanRateRefRatio" };
  std::string mGoodChanFracHistName{ "RatesSignal/LastCycle/GoodChannelsFraction" };
  std::string mGoodChanFracRatioHistName{ "RatesSignal/LastCycle/GoodChannelsFractionRefRatio" };
  int mMaxBadST12{ 2 };
  int mMaxBadST345{ 3 };
  double mMinRate{ 0.001 };
  double mMaxRate{ 10 };
  double mMaxRateDelta{ 0.2 };
  double mMinGoodFraction{ 0.9 };
  double mMaxGoodFractionDelta{ 0.2 };
  double mRatePlotScaleMin{ 0 };
  double mRatePlotScaleMax{ 10 };

  QualityChecker mQualityChecker;

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;

  ClassDefOverride(DigitsCheck, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DIGITSCHECK_H
