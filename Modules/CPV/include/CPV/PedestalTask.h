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
/// \file   PedestalTask.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_CPV_CPVPEDESTALTASK_H
#define QC_MODULE_CPV_CPVPEDESTALTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include <array>
#include "DataFormatsCPV/Digit.h"
#include "DataFormatsCPV/TriggerRecord.h"
#include <gsl/span>
#include "CPVBase/Geometry.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::cpv
{

/// \brief CPV Pedestal Task which processes uncalibrated digits from pedestal runs and produces pedestal monitor objects
/// \author Sergey Evdokimov
class PedestalTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  PedestalTask() = default;
  /// Destructor
  ~PedestalTask() override;

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
  // void fillHistograms(const gsl::span<const o2::cpv::Digit>& digits, const gsl::span<const o2::cpv::TriggerRecord>& triggerRecords);
  void fillHistograms();
  void resetHistograms();

  static constexpr short kNHist1D = 14;
  enum Histos1D { H1DInputPayloadSize,
                  H1DNInputs,
                  H1DNValidInputs,
                  H1DNDigitsPerInput,
                  H1DDigitIds,
                  H1DPedestalValueM2,
                  H1DPedestalValueM3,
                  H1DPedestalValueM4,
                  H1DPedestalSigmaM2,
                  H1DPedestalSigmaM3,
                  H1DPedestalSigmaM4,
                  H1DPedestalEfficiencyM2,
                  H1DPedestalEfficiencyM3,
                  H1DPedestalEfficiencyM4
  };

  static constexpr short kNHist2D = 16;
  enum Histos2D { H2DErrorType,
                  H2DDigitMapM2,
                  H2DDigitMapM3,
                  H2DDigitMapM4,
                  H2DPedestalValueMapM2,
                  H2DPedestalValueMapM3,
                  H2DPedestalValueMapM4,
                  H2DPedestalSigmaMapM2,
                  H2DPedestalSigmaMapM3,
                  H2DPedestalSigmaMapM4,
                  H2DPedestalEfficiencyMapM2,
                  H2DPedestalEfficiencyMapM3,
                  H2DPedestalEfficiencyMapM4,
                  H2DPedestalNPeaksMapM2,
                  H2DPedestalNPeaksMapM3,
                  H2DPedestalNPeaksMapM4
  };

  static constexpr short kNModules = 3;
  static constexpr short kNChannels = 23040;
  o2::cpv::Geometry mCPVGeometry;

  int mNEventsTotal;
  int mNEventsFromLastFillHistogramsCall;

  std::array<TH1F*, kNHist1D> mHist1D = { nullptr }; ///< Array of 1D histograms
  std::array<TH2F*, kNHist2D> mHist2D = { nullptr }; ///< Array of 2D histograms

  std::array<TH1F*, kNChannels> mHistAmplitudes = { nullptr };  ///< Array of amplitude spectra
  std::array<bool, kNChannels> mIsUpdatedAmplitude = { false }; ///< Array of isUpdatedAmplitude bools
};

} // namespace o2::quality_control_modules::cpv

#endif // QC_MODULE_CPV_CPVPEDESTALTASK_H
