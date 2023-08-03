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
#include "MCHRawElecMap/Mapper.h"
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
  PreclustersCheck();
  /// Destructor
  ~PreclustersCheck() override;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  std::array<Quality, getNumDE()> checkMeanEfficiencies(TH1F* h);
  std::array<Quality, getNumDE()> checkMeanEfficienciesRatio(TH1F* h);

  std::string mMeanEffHistNameB{ "Efficiency/LastCycle/MeanEfficiencyB" };
  std::string mMeanEffHistNameNB{ "Efficiency/LastCycle/MeanEfficiencyNB" };
  std::string mMeanEffRatioHistNameB{ "Efficiency/LastCycle/MeanEfficiencyRefRatioB" };
  std::string mMeanEffRatioHistNameNB{ "Efficiency/LastCycle/MeanEfficiencyRefRatioNB" };
  int mMaxBadST12{ 2 };
  int mMaxBadST345{ 3 };
  double mMinEfficiency{ 0.8 };
  double mMaxEffDelta{ 0.2 };
  double mPseudoeffPlotScaleMin{ 0.0 };
  double mPseudoeffPlotScaleMax{ 1.0 };

  QualityChecker mQualityChecker;

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  ClassDefOverride(PreclustersCheck, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
