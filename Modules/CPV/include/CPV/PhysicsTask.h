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

/// \brief Example Quality Control DPL Task
/// \author My Name
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
  //void fillHistograms(); // this will be needed in the future
  void resetHistograms();

  static constexpr short kNHist1D = 15;
  enum Histos1D { H1DInputPayloadSize,
                  H1DNInputs,
                  H1DNValidInputs,
                  H1DNDigitsPerInput,
                  H1DNClustersPerInput,
                  H1DDigitIds,
                  H1DDigitEnergyM2,
                  H1DDigitEnergyM3,
                  H1DDigitEnergyM4,
                  H1DClusterTotEnergyM2,
                  H1DClusterTotEnergyM3,
                  H1DClusterTotEnergyM4,
                  H1DNDigitsInClusterM2,
                  H1DNDigitsInClusterM3,
                  H1DNDigitsInClusterM4
  };

  static constexpr short kNHist2D = 6;
  enum Histos2D { H2DDigitMapM2,
                  H2DDigitMapM3,
                  H2DDigitMapM4,
                  H2DClusterMapM2,
                  H2DClusterMapM3,
                  H2DClusterMapM4
  };

  static constexpr short kNModules = 3;
  static constexpr short kNChannels = 23040;
  o2::cpv::Geometry mCPVGeometry;

  int mNEventsTotal;
  int mNEventsFromLastFillHistogramsCall;

  std::array<TH1F*, kNHist1D> mHist1D = { nullptr }; ///< Array of 1D histograms
  std::array<TH2F*, kNHist2D> mHist2D = { nullptr }; ///< Array of 2D histograms
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPHYSICSTASK_H
