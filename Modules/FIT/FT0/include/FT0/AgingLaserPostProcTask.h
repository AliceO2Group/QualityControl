// Copyright 2019-2025 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General
// Public License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   AgingLaserPostProcTask.h
/// \author Andreas Molander <andreas.molander@cern.ch>, Jakub Muszyński <jakub.milosz.muszynski@cern.ch>
/// \brief  Post-processing task that derives a per-channel
///         weighted-mean amplitude, normalised to reference channels.

#ifndef QC_MODULE_FT0_AGINGLASERPOSTPROC_H
#define QC_MODULE_FT0_AGINGLASERPOSTPROC_H

#include "QualityControl/PostProcessingInterface.h"
#include "FT0Base/Constants.h"
#include "FITCommon/PostProcHelper.h"

#include <TH1F.h>
#include <memory>
#include <vector>
#include <string>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

/// \brief Post-processing task that derives a per-channel
///        weighted-mean amplitude, normalised to reference channels.
class AgingLaserPostProcTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  AgingLaserPostProcTask() = default;
  ~AgingLaserPostProcTask() override;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  static constexpr std::size_t sNCHANNELS_PM = o2::ft0::Constants::sNCHANNELS_PM;

  std::string mAmpMoPath = "FT0/MO/AgingLaser"; //< Path to amplitude monitor object

  std::vector<uint8_t> mDetectorChIDs;  ///< Detector (target) channels
  std::vector<uint8_t> mReferenceChIDs; ///< Reference channels

  double mFracWindowA = 0.25;  ///< low fractional window parameter a
  double mFracWindowB = 0.25;  ///< high fractional window parameter b

  std::unique_ptr<TH1F> mAmpVsChNormWeightedMeanA;
  std::unique_ptr<TH1F> mAmpVsChNormWeightedMeanC;

  o2::quality_control_modules::fit::PostProcHelper mPostProcHelper;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_AGINGLASERPOSTPROC_H