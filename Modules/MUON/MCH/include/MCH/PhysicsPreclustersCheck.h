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
/// \file   PhysicsCheck.h
/// \author Andrea Ferrero, Sebastien Perrin
///

#ifndef QC_MODULE_MCH_PHYSICSPRECLUSTERSCHECK_H
#define QC_MODULE_MCH_PHYSICSPRECLUSTERSCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"
#include <string>

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check if the occupancy from trending is between the two specified values
///
/// \author Andrea Ferrero, Sebastien Perrin
class PhysicsPreclustersCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PhysicsPreclustersCheck();
  /// Destructor
  ~PhysicsPreclustersCheck() override;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  int checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel, int& cathode);

  double mMinPseudoeff;
  double mMaxPseudoeff;
  double mMinGoodFraction;
  double mPseudoeffPlotScaleMin;
  double mPseudoeffPlotScaleMax;
  bool mVerbose;

  std::vector<double> mDePseudoeff[2];

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  ClassDefOverride(PhysicsPreclustersCheck, 4);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
