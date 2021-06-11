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
/// \file   RawQcTask.h
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#ifndef QC_MODULE_PHOS_PHOSQCRAWTASK_H
#define QC_MODULE_PHOS_PHOSQCRAWTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include <array>
#include "DataFormatsPHOS/Cell.h"
#include "DataFormatsPHOS/TriggerRecord.h"
#include <gsl/span>

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::phos
{

/// \brief PHOS Quality Control DPL Task
class RawQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  RawQcTask() = default;
  /// Destructor
  ~RawQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 protected:
  static constexpr short kNhist1D = 21;
  enum histos1D { kTotalDataVolume,
                  kMessageCounter,
                  kHGmeanSummaryM1,
                  kHGmeanSummaryM2,
                  kHGmeanSummaryM3,
                  kHGmeanSummaryM4,
                  kLGmeanSummaryM1,
                  kLGmeanSummaryM2,
                  kLGmeanSummaryM3,
                  kLGmeanSummaryM4,
                  kHGrmsSummaryM1,
                  kHGrmsSummaryM2,
                  kHGrmsSummaryM3,
                  kHGrmsSummaryM4,
                  kLGrmsSummaryM1,
                  kLGrmsSummaryM2,
                  kLGrmsSummaryM3,
                  kLGrmsSummaryM4,
                  kCellSpM1,
                  kCellSpM2,
                  kCellSpM3,
                  kCellSpM4
  };

  static constexpr short kNhist2D = 35;
  enum histos2D { kErrorType,
                  kPayloadSizePerDDL,
                  kHGmeanM1,
                  kHGmeanM2,
                  kHGmeanM3,
                  kHGmeanM4,
                  kLGmeanM1,
                  kLGmeanM2,
                  kLGmeanM3,
                  kLGmeanM4,
                  kHGrmsM1,
                  kHGrmsM2,
                  kHGrmsM3,
                  kHGrmsM4,
                  kLGrmsM1,
                  kLGrmsM2,
                  kLGrmsM3,
                  kLGrmsM4,
                  kHGoccupM1,
                  kHGoccupM2,
                  kHGoccupM3,
                  kHGoccupM4,
                  kLGoccupM1,
                  kLGoccupM2,
                  kLGoccupM3,
                  kLGoccupM4,
                  kCellOccupM1,
                  kCellOccupM2,
                  kCellOccupM3,
                  kCellOccupM4,
                  kCellEM1,
                  kCellEM2,
                  kCellEM3,
                  kCellEM4,
                  kTimeEM1,
                  kTimeEM2,
                  kTimeEM3,
                  kTimeEM4 };

  void InitHistograms();

  void CreatePhysicsHistograms();
  void FillPhysicsHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& tr);
  void CreatePedestalHistograms();
  void FillPedestalHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& tr);
  void CreateLEDHistograms() {}
  void FillLEDHistograms(const gsl::span<const o2::phos::Cell>& /*cells*/, const gsl::span<const o2::phos::TriggerRecord>& /*tr*/) {}

 private:
  static constexpr short kNmod = 6;
  static constexpr short kMaxErr = 5;
  static constexpr short kOcccupancyTh = 10;

  int mMode = 0;           ///< Possible modes: 0(def): Physics, 1: Pedestals, 2: LED
  bool mFinalized = false; ///< if final histograms calculated

  std::array<TH1F*, kNhist1D> mHist1D = { nullptr }; ///< Array of 1D histograms
  std::array<TH2F*, kNhist2D> mHist2D = { nullptr }; ///< Array of 2D histograms
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_PHOSRAWQCTASK_H
