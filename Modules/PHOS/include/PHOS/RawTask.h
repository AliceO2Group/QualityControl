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
/// \file   RawTask.h
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#ifndef QC_MODULE_PHOS_PHOSRAWTASK_H
#define QC_MODULE_PHOS_PHOSRAWTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>
#include <array>

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::phos
{

/// \brief PHOS Quality Control DPL Task
class RawTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  RawTask() = default;
  /// Destructor
  ~RawTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  static constexpr short mNmod = 5;
  TH1F* mHistogram = nullptr;
  TH1F* mMessageCounter = nullptr;
  TH1F* mNumberOfSuperpagesPerMessage;
  TH1F* mNumberOfPagesPerMessage;             ///<
  TH1F* mSuperpageCounter = nullptr;          ///< Counter for number of superpages
  TH1F* mPageCounter = nullptr;               ///< Counter for number of pages (headers)
  TH1F* mTotalDataVolume = nullptr;           ///< Total data volume
  std::array<TH1F*, mNmod> mRawAmplitudePHOS; ///< Raw amplitude in PHOS
  std::array<TH1F*, mNmod> mRawAmplMaxPHOS;   ///< Max Raw amplitude in PHOS per cell
  std::array<TH1F*, mNmod> mRawAmplMinPHOS;   ///< Min Raw amplitude in PHOS per cell
  std::array<TH2F*, mNmod> mRMSperMod;        ///< ADC rms per SM
  std::array<TH2F*, mNmod> mMEANperMod;       ///< ADC mean per SM
  std::array<TH2F*, mNmod> mMAXperMod;        ///< ADC max per SM
  std::array<TH2F*, mNmod> mMINperMod;        ///< ADC min per SM
  TH2F* mErrorTypeAltro = nullptr;            ///< Error from AltroDecoder
  TH2F* mPayloadSizePerDDL = nullptr;         ///< Payload size per ddl
  int mNumberOfSuperpages = 0;                ///< Simple total superpage counter
  int mNumberOfPages = 0;                     ///< Simple total number of superpages counter
  int mNumberOfMessages = 0;
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_PHOSRAWTASK_H
