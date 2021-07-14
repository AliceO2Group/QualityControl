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
/// \file   TaskCosmics.h
/// \author Nicolo' Jacazio
/// \brief  Task to monitor TOF data collected in events from cosmics
///

#ifndef QC_MODULE_TOF_TASK_COSMICS_H
#define QC_MODULE_TOF_TASK_COSMICS_H

#include "QualityControl/TaskInterface.h"
#include "Base/Counter.h"
#include "TOFBase/Geo.h"

class TH1F;
class TH2F;
class TH1I;
class TH2I;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control DPL Task
/// \author Nicolo' Jacazio
class TaskCosmics final : public TaskInterface
{
 public:
  /// \brief Constructor
  TaskCosmics();
  /// Destructor
  ~TaskCosmics() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  // Parameters
  const float nrow = 256 * 3;
  const float mTFDuration = ((nrow - 1) / nrow) * o2::tof::Geo::BC_TIME * o2::tof::Geo::BC_IN_ORBIT * 256 * 1E-9; /// Duration of a TF used to compute the rate of cosmics
  float mSelDeltaTSignalRegion = 50000.f;                                                                         /// Cut on the DeltaT to select signal
  float mSelDeltaTBackgroundRegion = 100000.f;                                                                    /// Cut on the DeltaT to select background
  float mSelMinLength = 500.f;                                                                                    /// Cut on the minimum length that a track mush have [cm]

  // Histograms
  std::shared_ptr<TH1F> mHistoCrate1 = nullptr;         /// Crates of the first hit
  std::shared_ptr<TH1F> mHistoCrate2 = nullptr;         /// Crates of the second hit
  std::shared_ptr<TH2F> mHistoCrate1VsCrate2 = nullptr; /// Crates of the second hit
  std::shared_ptr<TH1F> mHistoDeltaT = nullptr;         /// DeltaT
  std::shared_ptr<TH1F> mHistoToT1 = nullptr;           /// ToT1
  std::shared_ptr<TH1F> mHistoToT2 = nullptr;           /// ToT2
  std::shared_ptr<TH1F> mHistoLength = nullptr;         /// Length
  std::shared_ptr<TH2F> mHistoDeltaTLength = nullptr;   /// DeltaT vs Length
  std::shared_ptr<TH1F> mHistoCosmicRate = nullptr;     /// Rate of cosmics per crate

  // Counters
  Counter<2, nullptr> mCounterTF;    /// Counter for the number of TF seen
  Counter<72, nullptr> mCounterPeak; /// Counter for coincidence between signals in the peak region
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TASK_COSMICS_H
