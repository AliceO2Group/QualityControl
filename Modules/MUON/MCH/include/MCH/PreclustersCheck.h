// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PreclustersCheck.h
/// \author Andrea Ferrero, Sebastien Perrin
///

#ifndef QC_MODULE_MCH_PHYSICSPRECLUSTERSCHECK_H
#define QC_MODULE_MCH_PHYSICSPRECLUSTERSCHECK_H

#include "MCH/Helpers.h"
#include "QualityControl/CheckInterface.h"
#include "QualityControl/Quality.h"
#include <string>

namespace o2::quality_control::core
{
class MonitorObject;
}

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Checker for the pre-clusters plots
///
/// \author Andrea Ferrero, Sebastien Perrin
class PreclustersCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PreclustersCheck() = default;
  /// Destructor
  ~PreclustersCheck() override = default;

  // Override interface
  void configure() override;
  void startOfActivity(const Activity& activity) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override; private:
  std::array<Quality, getNumDE()> checkMeanEfficiencies(TH1* h);
  std::array<Quality, getNumDE()> checkMeanEfficiencyRatios(TH1* h);
  void checkSolarMeanEfficiencies(TH1* h);
  void checkSolarMeanEfficiencyRatios(TH1* h);

  std::string mMeanEffHistNameB{ "Efficiency/MeanEfficiencyB" };
  std::string mMeanEffHistNameNB{ "Efficiency/MeanEfficiencyNB" };
  std::string mMeanEffPerSolarHistName{ "Efficiency/MeanEfficiencyPerSolar" };
  std::string mMeanEffRefCompHistNameB{ "Efficiency/RefComp/MeanEfficiencyB" };
  std::string mMeanEffRefCompHistNameNB{ "Efficiency/RefComp/MeanEfficiencyNB" };
  std::string mMeanEffPerSolarRefCompHistName{ "Efficiency/RefComp/MeanEfficiencyPerSolar" };
  int mMaxBadST12{ 2 };
  int mMaxBadST345{ 3 };
  double mMinEfficiency{ 0.8 };
  std::array<std::optional<double>, 5> mMinEfficiencyPerStation;
  double mMinEfficiencyPerSolar{ 0.5 };
  double mMinEfficiencyRatio{ 0.9 };
  double mMinEfficiencyRatioPerSolar{ 0.9 };
  double mPseudoeffPlotScaleMin{ 0.0 };
  double mPseudoeffPlotScaleMax{ 1.05 };
  double mEfficiencyRatioScaleRange{ 0.2 };
  double mEfficiencyRatioPerSolarScaleRange{ 0.2 };

  QualityChecker mQualityChecker;
  std::array<Quality, getNumSolar()> mSolarQuality;

  ClassDefOverride(PreclustersCheck, 3);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
