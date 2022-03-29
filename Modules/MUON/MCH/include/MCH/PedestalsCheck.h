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
/// \file   PedestalsCheck.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MCH_PEDESTALSCHECK_H
#define QC_MODULE_MCH_PEDESTALSCHECK_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"

namespace o2::quality_control_modules::muonchambers
{

class PedestalsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PedestalsCheck();
  /// Destructor
  ~PedestalsCheck() override;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  /// check if a given electronics channel is associated with a detector pad
  bool checkPadMapping(uint16_t feeId, uint8_t linkId, uint8_t eLinkId, o2::mch::raw::DualSampaChannelId channel);

  /// Minimum value for SAMPA pedestals
  float mMinPedestal;
  /// Maximum value for SAMPA pedestals
  float mMaxPedestal;
  /// Minimum fraction of good channels for "good" quality status
  float mMinGoodFraction;

  /// direct and inverse electronics and detector mappings
  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  ClassDefOverride(PedestalsCheck, 3);
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
