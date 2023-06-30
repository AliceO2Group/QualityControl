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

#include <TH1.h>
class TH2F;
#include "DataFormatsPHOS/BadChannelsMap.h"
#include <TSpectrum.h>
#include "PHOS/TH2FMean.h"
#include "PHOS/TH2SBitmask.h"
#include "PHOS/TH1Fraction.h"

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
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 protected:
  static constexpr short kNhist1D = 27;
  enum histos1D { kTotalDataVolume,
                  kMessageCounter,
                  kBadMapSummary,
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
                  kCellHGSpM1,
                  kCellHGSpM2,
                  kCellHGSpM3,
                  kCellHGSpM4,
                  kCellLGSpM1,
                  kCellLGSpM2,
                  kCellLGSpM3,
                  kCellLGSpM4
  };

  static constexpr short kNhist2D = 42;
  enum histos2D { kErrorNumber,
                  kPayloadSizePerDDL,
                  kChi2M1,
                  kChi2M2,
                  kChi2M3,
                  kChi2M4,
                  kChi2NormM1,
                  kChi2NormM2,
                  kChi2NormM3,
                  kChi2NormM4,
                  kHGoccupM1,
                  kHGoccupM2,
                  kHGoccupM3,
                  kHGoccupM4,
                  kLGoccupM1,
                  kLGoccupM2,
                  kLGoccupM3,
                  kLGoccupM4,
                  kTimeEM1,
                  kTimeEM2,
                  kTimeEM3,
                  kTimeEM4,
                  kTRUSTOccupM1,
                  kTRUSTOccupM2,
                  kTRUSTOccupM3,
                  kTRUSTOccupM4,
                  kTRUDGOccupM1,
                  kTRUDGOccupM2,
                  kTRUDGOccupM3,
                  kTRUDGOccupM4,
                  kTRUSTMatchM1,
                  kTRUSTMatchM2,
                  kTRUSTMatchM3,
                  kTRUSTMatchM4,
                  kTRUSTFakeM1,
                  kTRUSTFakeM2,
                  kTRUSTFakeM3,
                  kTRUSTFakeM4,
                  kTRUDGFakeM1,
                  kTRUDGFakeM2,
                  kTRUDGFakeM3,
                  kTRUDGFakeM4 };

  static constexpr short kNhist2DMean = 24;
  enum histos2DMean { kHGmeanM1,
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
                      kCellEM1,
                      kCellEM2,
                      kCellEM3,
                      kCellEM4,
                      kLEDNpeaksM1,
                      kLEDNpeaksM2,
                      kLEDNpeaksM3,
                      kLEDNpeaksM4
  };

  static constexpr short kNhist2DBitmask = 1;
  enum histos2DBitmask { kErrorType };

  static constexpr short kNratio1D = 1;
  enum ratios1D { kErrorTypeOccurance };

  void InitHistograms();

  void CreatePhysicsHistograms();
  void FillPhysicsHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& tr);
  void CreatePedestalHistograms();
  void FillPedestalHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& tr);
  void CreateLEDHistograms();
  void FillLEDHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& tr);

  void CreateTRUHistograms();
  void FillTRUHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& tr);

 private:
  static constexpr short kNmod = 6;
  static constexpr short kMaxErr = 5;
  static constexpr short kOcccupancyTh = 10;

  int mMode = 0;           ///< Possible modes: 0(def): Physics, 1: Pedestals, 2: LED
  bool mFinalized = false; ///< if final histograms calculated
  bool mCheckChi2 = false; ///< scan Chi2 distributions
  bool mTrNoise = false;   ///< check mathing of trigger summary tables and tr.digits
  unsigned long long eventCounter = 0;

  std::array<TH1F*, kNhist1D> mHist1D = { nullptr };                      ///< Array of 1D histograms
  std::array<TH2F*, kNhist2D> mHist2D = { nullptr };                      ///< Array of 2D histograms
  std::array<TH2FMean*, kNhist2DMean> mHist2DMean = { nullptr };          ///< Array of 2D mean histograms
  std::array<TH2SBitmask*, kNhist2DBitmask> mHist2DBitmask = { nullptr }; ///< Array of 2D mean histograms
  std::array<TH1Fraction*, kNratio1D> mFractions1D = { nullptr };         ///< Array of mergeable 1D fractions

  bool mInitBadMap = true;                           //! BadMap had to be initialized
  const o2::phos::BadChannelsMap* mBadMap = nullptr; //! Bad map for comparison
  std::unique_ptr<TSpectrum> mSpSearcher;
  std::vector<TH1S> mSpectra;
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_PHOSRAWQCTASK_H
