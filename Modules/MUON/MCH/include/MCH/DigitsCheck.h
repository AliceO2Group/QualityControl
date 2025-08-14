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
#include "QualityControl/CheckInterface.h"
#include "QualityControl/Quality.h"
#include <string>
#include <array>

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
  void startOfActivity(const Activity& activity) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override; private:
  std::array<Quality, getNumDE()> checkMeanRates(TH1* h);
  std::array<Quality, getNumDE()> checkBadChannels(TH1* h);
  std::array<Quality, getNumDE()> checkMeanRateRatios(TH1* h);
  std::array<Quality, getNumDE()> checkBadChannelRatios(TH1* h);
  void checkSolarMeanRates(TH1* h);
  void checkSolarBadChannels(TH1* h);
  void checkSolarMeanRateRatios(TH1* h);
  void checkSolarBadChannelRatios(TH1* h);

  std::string mMeanRateHistName{ "RatesSignal/MeanRate" };
  std::string mGoodChanFracHistName{ "RatesSignal/GoodChannelsFraction" };
  std::string mMeanRatePerSolarHistName{ "RatesSignal/MeanRatePerSolar" };
  std::string mGoodChanFracPerSolarHistName{ "RatesSignal/GoodChannelsFractionPerSolar" };
  std::string mMeanRateRefCompHistName{ "RatesSignal/RefComp/MeanRate" };
  std::string mGoodChanFracRefCompHistName{ "RatesSignal/RefComp/GoodChannelsFraction" };
  std::string mMeanRatePerSolarRefCompHistName{ "RatesSignal/RefComp/MeanRatePerSolar" };
  std::string mGoodChanFracPerSolarRefCompHistName{ "RatesSignal/RefComp/GoodChannelsFractionPerSolar" };
  int mMaxBadST12{ 2 };
  int mMaxBadST345{ 3 };

  // Rate lower threshold
  double mMinRate{ 0.001 };
  std::array<std::optional<double>, 5> mMinRatePerStation;
  double mMinRatePerSolar{ 0.001 };
  // Rate upper threshold
  double mMaxRate{ 10 };
  std::array<std::optional<double>, 5> mMaxRatePerStation;
  double mMaxRatePerSolar{ 10 };
  // Rate ratio threshold
  double mMinRateRatio{ 0.9 };
  double mMinRateRatioPerSolar{ 0.9 };

  // Good channels fraction threshold
  double mMinGoodFraction{ 0.9 };
  std::array<std::optional<double>, 5> mMinGoodFractionPerStation;
  double mMinGoodFractionPerSolar{ 0.5 };
  // Good channels ratio threshold
  double mMinGoodFractionRatio{ 0.9 };
  double mMinGoodFractionRatioPerSolar{ 0.9 };

  // Vertical plot ranges
  double mRatePlotScaleMin{ 0 };
  double mRatePlotScaleMax{ 10 };
  double mRateRatioPlotScaleRange{ 0.2 };
  double mRateRatioPerSolarPlotScaleRange{ 0.2 };
  double mGoodFractionRatioPlotScaleRange{ 0.2 };
  double mGoodFractionRatioPerSolarPlotScaleRange{ 0.2 };

  QualityChecker mQualityChecker;
  std::array<Quality, getNumSolar()> mSolarQuality;

  ClassDefOverride(DigitsCheck, 3);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_DIGITSCHECK_H
