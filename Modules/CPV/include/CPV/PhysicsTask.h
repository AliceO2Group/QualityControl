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
/// \file   PhysicsTask.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_CPV_CPVPHYSICSTASK_H
#define QC_MODULE_CPV_CPVPHYSICSTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include <array>
#include <gsl/span>
#include "CPVBase/Geometry.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::cpv
{

/// \brief Task for CPV Physics monitoring
/// \author Sergey Evdokimov
using Geometry = o2::cpv::Geometry;
class PhysicsTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  PhysicsTask() = default;
  /// Destructor
  ~PhysicsTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void initHistograms();
  void resetHistograms();

  static constexpr short kNHist1D = 29;
  enum Histos1D { H1DInputPayloadSize,
                  H1DNInputs,
                  H1DNValidInputs,
                  H1DRawErrors,
                  H1DNDigitsPerInput,
                  H1DNClustersPerInput,
                  H1DNCalibDigitsPerInput,
                  H1DDigitIds,
                  H1DCalibDigitIds,
                  H1DDigitsInEventM2,
                  H1DDigitsInEventM3,
                  H1DDigitsInEventM4,
                  H1DDigitsInEventM2M3M4,
                  H1DDigitEnergyM2,
                  H1DDigitEnergyM3,
                  H1DDigitEnergyM4,
                  H1DCalibDigitEnergyM2,
                  H1DCalibDigitEnergyM3,
                  H1DCalibDigitEnergyM4,
                  H1DClustersInEventM2,
                  H1DClustersInEventM3,
                  H1DClustersInEventM4,
                  H1DClustersInEventM2M3M4,
                  H1DClusterTotEnergyM2,
                  H1DClusterTotEnergyM3,
                  H1DClusterTotEnergyM4,
                  H1DNDigitsInClusterM2,
                  H1DNDigitsInClusterM3,
                  H1DNDigitsInClusterM4
  };

  static constexpr short kNHist2D = 24;
  enum Histos2D { H2DDigitMapM2,
                  H2DDigitMapM3,
                  H2DDigitMapM4,
                  H2DCalibDigitMapM2,
                  H2DCalibDigitMapM3,
                  H2DCalibDigitMapM4,
                  H2DDigitFreqM2,
                  H2DDigitFreqM3,
                  H2DDigitFreqM4,
                  H2DClusterMapM2,
                  H2DClusterMapM3,
                  H2DClusterMapM4,
                  H2DPedestalValueM2,
                  H2DPedestalValueM3,
                  H2DPedestalValueM4,
                  H2DPedestalSigmaM2,
                  H2DPedestalSigmaM3,
                  H2DPedestalSigmaM4,
                  H2DBadChannelMapM2,
                  H2DBadChannelMapM3,
                  H2DBadChannelMapM4,
                  H2DGainsM2,
                  H2DGainsM3,
                  H2DGainsM4
  };

  static constexpr short kNModules = 3;
  static constexpr short kNChannels = 23040;
  int mNEventsTotal = 0;
  int mCcdbCheckIntervalInMinutes = 10;
  std::array<TH1F*, kNHist1D> mHist1D = { nullptr }; ///< Array of 1D histograms
  std::array<TH2F*, kNHist2D> mHist2D = { nullptr }; ///< Array of 2D histograms
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPHYSICSTASK_H
