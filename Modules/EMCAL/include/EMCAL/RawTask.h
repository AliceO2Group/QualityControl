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
/// \author Cristina Terrevoli
/// \author Markus Fasel
///

#ifndef QC_MODULE_EMCAL_EMCALRAWTASK_H
#define QC_MODULE_EMCAL_EMCALRAWTASK_H

#include "QualityControl/TaskInterface.h"
#include "EMCALBase/Mapper.h"
#include <memory>
#include <array>

class TH1F;
class TH2F;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

/// \brief Example Quality Control DPL Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
/// \author Piotr Konopka
class RawTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
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
  TH1F* mHistogram = nullptr;
  TH1* mMessageCounter = nullptr;
  TH1* mNumberOfSuperpagesPerMessage;
  TH1* mNumberOfPagesPerMessage;
  TH1* mSuperpageCounter = nullptr;                     ///< Counter for number of superpages
  TH1* mPageCounter = nullptr;                          ///< Counter for number of pages (headers)
  TH1* mTotalDataVolume = nullptr;                      ///< Total data volume
  std::array<TH1*, 20> mRawAmplitudeEMCAL;              ///< Raw amplitude in EMCAL
  std::array<TH1*, 20> mRawAmplMaxEMCAL;                ///< Max Raw amplitude in EMCAL per cell
  std::array<TH1*, 20> mRawAmplMinEMCAL;                ///< Min Raw amplitude in EMCAL per cell
  std::unique_ptr<o2::emcal::MappingHandler> mMappings; ///< Mappings Hardware address -> Channel
  std::array<TProfile2D*, 20> mRMSperSM;                ///< ADC rms per SM
  std::array<TProfile2D*, 20> mMEANperSM;               ///< ADC mean per SM
  std::array<TProfile2D*, 20> mMAXperSM;                ///< ADC max per SM
  std::array<TProfile2D*, 20> mMINperSM;                ///< ADC min per SM
  TH2F* mErrorTypeAltro = nullptr;                      ///< Error from AltroDecoder
  TH2F* mPayloadSizePerDDL = nullptr;                   ///< Payload size per ddl
  Int_t mNumberOfSuperpages = 0;                        ///< Simple total superpage counter
  Int_t mNumberOfPages = 0;                             ///< Simple total number of superpages counter
  Int_t mNumberOfMessages = 0;
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWTASK_H
