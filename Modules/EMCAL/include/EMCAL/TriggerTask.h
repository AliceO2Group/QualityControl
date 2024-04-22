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
/// \file   TriggerTask.h
/// \author Markus Fasel
///

#ifndef QC_MODULE_EMCAL_EMCALTRIGGERTASK_H
#define QC_MODULE_EMCAL_EMCALTRIGGERTASK_H

#include "QualityControl/TaskInterface.h"
#include "EMCALBase/TriggerMappingV2.h"
#include <gsl/span>
#include <memory>
#include <vector>

class TH1;
class TH2;
class TProfile2D;

namespace o2
{
class InteractionRecord;
}

namespace o2::emcal
{
class CompressedTRU;
class CompressedTriggerPatch;
class CompressedL0TimeSum;
class TriggerRecord;
}; // namespace o2::emcal

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

/// \class TriggerTask
/// \brief Task monitoring EMCAL trigger observables
/// \author Markus Fasel <markus.fasel@cern.ch>, Oak Ridge National Laboratory
/// \since April 19, 2024
///
/// Task monitoring basic observables of the EMCAL trigger system. The task subscibes to:
///
/// ||   Binding    ||     Channel    ||            Information              ||
/// |---------------|-----------------|---------------------------------------|
/// | truinfo       | EMC/TRUS        | TRU data                              |
/// | trurecords    | EMC/TRUSTRGR    | Trigger records of TRU data           |
/// | patchinfos    | EMC/PATCHES     | Trigger patches                       |
/// | patchrecords  | EMC/PATCHESTRGR | Trigger records of trigger patches    |
/// | timesums      | EMC/FASTORS     | L0 FastOR timesums                    |
/// |timesumrecords | EMC/FASTORSTRGR | Trigger records of L0 FastOR timesums |
///
/// Monitoring per timeframe is done in the function monitorData, which delegates
/// the monitoring per trigger to an internal function processEvent.
///
/// The task defines the following task parameters:
/// - None so far
class TriggerTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  TriggerTask() = default;
  /// \brief Destructor
  ~TriggerTask() override;

  /// \brief Initialize task (histograms and trigger mapping)
  /// \param ctx InitContenxt
  void initialize(o2::framework::InitContext& ctx) override;

  /// \brief Operations performed at the start of activity (start of run)
  /// \param activity Activity
  void startOfActivity(const Activity& activity) override;

  /// \brief Operations at performed at the start of a new monitoring cycle
  void startOfCycle() override;

  /// \brief Monitoring data for a given timeframe
  /// \param ctx Processing context with data
  ///
  /// As the data in EMCAL is organized in triggers within the container the
  /// monitoring is delegated to the internal function processEvent. monitorData
  /// subscribes to the timeframe-based container of the TRU information, trigger
  /// patches and timesums, as well as their corresponding trigger records. Iteration
  /// over all BCs found in any of the containers via getAllBCs is performed, and
  /// for each trigger the TRU, patch and FastOR information is extracted from the
  /// timeframe-based containers.
  void monitorData(o2::framework::ProcessingContext& ctx) override;

  /// \brief Operations performed at the end of the current monitoring cycle
  void endOfCycle() override;

  /// \brief Operations performed at the end fo the activity (end of run)
  /// \param activity Activity
  void endOfActivity(const Activity& activity) override;

  /// \brief Reset all histograms
  void reset() override;

 private:
  /// \brief Fill monitoring histograms for a single event
  /// \param trudata TRU data for the event
  /// \param triggerpatches Trigger patches found in the event
  /// \param timesums L0 timesums found in the event
  void processEvent(const gsl::span<const o2::emcal::CompressedTRU> trudata, const gsl::span<const o2::emcal::CompressedTriggerPatch> triggerpatches, const gsl::span<const o2::emcal::CompressedL0TimeSum> timesums);

  /// \brief Find all BCs with data in either of the components (or all)
  /// \param trurecords Trigger records for TRU data
  /// \param patchrecords Trigger records for trigger patches
  /// \param timesumrecords Trigger records for L0 timesums
  /// \return List of found BCs (sorted)
  std::vector<o2::InteractionRecord> getAllBCs(const gsl::span<const o2::emcal::TriggerRecord> trurecords, const gsl::span<const o2::emcal::TriggerRecord> patchrecords, const gsl::span<const o2::emcal::TriggerRecord> timesumrecords) const;

  std::unique_ptr<o2::emcal::TriggerMappingV2> mTriggerMapping; ///< Trigger mapping
  TH1* mTRUFired = nullptr;                                     ///< Histogram with counters per TRU fired
  TH1* mFastORFired = nullptr;                                  ///< Histogram with counters per FastOR fired (in patches)
  TH2* mPositionFasORFired = nullptr;                           ///< Histogram with the position of fired FastORs (in patches)
  TH1* mNumberOfTRUsPerEvent = nullptr;                         ///< Counter histogram for number of fired TRUs per event
  TH1* mNumberOfPatchesPerEvent = nullptr;                      ///< Counter histogram for number of fired patches per event
  TH1* mPatchEnergySpectrumPerEvent = nullptr;                  ///< Histogram for integrated patch energy spectrum
  TH1* mLeadingPatchEnergySpectrumPerEvent = nullptr;           ///< Histogram for integrated leading patch energy spectrum
  TH2* mPatchEnergyTRU = nullptr;                               ///< Histogram for patch energy spectrum per TRU
  TH2* mLeadingPatchEnergyTRU = nullptr;                        ///< Histogram for leading patch energy spectrum per TRU
  TH2* mNumberOfPatchesPerTRU = nullptr;                        ///< Counter histogram for number of patches per TRU
  TH2* mPatchIndexFired = nullptr;                              ///< Histogram with the fired patch index per TRU
  TH2* mPatchIndexLeading = nullptr;                            ///< Histogram with the leading fired patch index per TRU
  TH2* mTRUTime = nullptr;                                      ///< Histogram with TRU time vs TRU index
  TH2* mPatchTime = nullptr;                                    ///< Histogram with patch time vs. TRU index
  TH2* mLeadingPatchTime = nullptr;                             ///< Histogram with patch time of the leading patch vs. TRU index
  TH1* mNumberTimesumsEvent = nullptr;                          ///< Counter histogram with number of non-0 FastOR timesums per event
  TH1* mL0Timesums = nullptr;                                   ///< ADC spectrum of the FastOR timesums
  TH2* mL0TimesumsTRU = nullptr;                                ///< ADC spectrum of the the FastOR timesums per TRU
  TH1* mADCMaxTimesum = nullptr;                                ///< ADC spectrum of the leading FastOR timesum per event
  TH1* mFastORIndexMaxTimesum = nullptr;                        ///< Index of the leading FastOR timesum per event
  TH2* mPositionMaxTimesum = nullptr;                           ///< Position of teh leading FastOR timsum per event
  TH2* mIntegratedTimesums = nullptr;                           ///< Integrated ADC timesum
  TProfile2D* mAverageTimesum = nullptr;                        ///< Average ADC timesum
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALTRIGGERTASK_H
