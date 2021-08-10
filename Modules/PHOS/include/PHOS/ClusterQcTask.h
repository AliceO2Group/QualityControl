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
/// \file   ClusterQcTask.h
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#ifndef QC_MODULE_PHOS_QCCLUSTERTASK_H
#define QC_MODULE_PHOS_QCCLUSTERTASK_H

#include "QualityControl/TaskInterface.h"
#include "DataFormatsPHOS/Cluster.h"
#include "DataFormatsPHOS/BadChannelsMap.h"
#include "PHOSBase/Geometry.h"
#include <TLorentzVector.h>
#include <memory>
#include <array>

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::phos
{

/// \brief PHOS Quality Control DPL Task
class ClusterQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ClusterQcTask() = default;
  /// Destructor
  ~ClusterQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 protected:
  bool checkCluster(const o2::phos::Cluster& c);

 private:
  static constexpr short kNhist1D = 8;
  enum histos1D { kSpectrumM1,
                  kSpectrumM2,
                  kSpectrumM3,
                  kSpectrumM4,
                  kPi0M1,
                  kPi0M2,
                  kPi0M3,
                  kPi0M4

  };
  static constexpr short kNhist2D = 8;
  enum histos2D { kOccupancyM1,
                  kOccupancyM2,
                  kOccupancyM3,
                  kOccupancyM4,
                  kTimeEM1,
                  kTimeEM2,
                  kTimeEM3,
                  kTimeEM4
  };
  float mPtMin = 1.5;                                /// minimum pi0 pt to fill inv mass histo
  float mOccCut = 0.1;                               /// minimum energy to fill occupancy histo
  std::array<TH1F*, kNhist1D> mHist1D = { nullptr }; ///< Array of 1D histograms
  std::array<TH2F*, kNhist2D> mHist2D = { nullptr }; ///< Array of 2D histograms
  std::vector<TLorentzVector> mBuffer[4];            /// Keep photons per event per module
  o2::phos::Geometry* mGeom;                         /// Pointer to PHOS singleton geometry
  std::unique_ptr<o2::phos::BadChannelsMap> mBadMap; /// bad map
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_QCCLUSTERTASK_H
