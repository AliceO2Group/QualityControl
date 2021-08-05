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
#include <cstdint>
#include <unordered_map>
#include <string_view>

#include "DetectorsRaw/RDHUtils.h"
#include "Headers/RAWDataHeader.h"
#include <Framework/InputRecord.h>
#include <CommonConstants/Triggers.h>

class TH1F;
class TH2F;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::emcal
{
class Geometry;
}

namespace o2::quality_control_modules::emcal
{

/// \brief Example Quality Control DPL Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
/// \author Piotr Konopka
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

  /// \brief Set the data origin
  /// \param origin Data origin
  ///
  /// Normally data origin is EMC, however in case the
  /// Task subscribes directly to readout or the origin
  /// is different in the STFbuilder this needs to be handled
  /// accordingly
  void setDataOrigin(const std::string_view origin) { mDataOrigin = origin; }

  enum class EventType {
    CAL_EVENT,
    PHYS_EVENT
  };

 private:
  /// \struct RawEventType
  /// \brief Key type for maps caching event information from different subevents, containing also trigger information
  struct RawEventType {
    o2::InteractionRecord mIR;
    uint32_t mTrigger;
    bool operator==(const RawEventType& other) const { return mIR == other.mIR; }
    bool operator<(const RawEventType& other) const { return mIR < other.mIR; }
  };

  /// \struct RawEventTypeHash
  /// \brief Hash-value calculation for struct RawEventType, used in std::unordered_map
  struct RawEventTypeHash {
    /// \brief Hash function, purely based on bc and orbit ID as they are unique for a collision
    /// \param evtype Raw event information with event interaction record
    /// \return Hash value
    std::size_t operator()(const RawEventType& evtype) const
    {
      size_t h1 = std::hash<int>()(evtype.mIR.bc);
      size_t h2 = std::hash<int>()(evtype.mIR.orbit);
      return h1 ^ (h2 << 1);
    }
  };

  bool isLostTimeframe(framework::ProcessingContext& ctx) const;

  o2::emcal::Geometry* mGeometry = nullptr; ///< EMCAL geometry
  std::string mDataOrigin = "EMC";
  TH1* mPayloadSize = nullptr;
  TH1* mMessageCounter = nullptr;
  TH1* mNumberOfSuperpagesPerMessage;
  TH1* mNumberOfPagesPerMessage;
  TH1* mSuperpageCounter = nullptr;                                          ///< Counter for number of superpages
  TH1* mPageCounter = nullptr;                                               ///< Counter for number of pages (headers)
  TH1* mTotalDataVolume = nullptr;                                           ///< Total data volume
  TH1* mNbunchPerChan = nullptr;                                             ///< Number of bunch per Channel
  TH1* mNofADCsamples = nullptr;                                             ///< Number of ADC samples per Channel
  TH1* mADCsize = nullptr;                                                   ///< ADC size per bunch
  std::array<TH1*, 20> mFECmaxCount;                                         ///< max number of hit channels
  std::array<TH1*, 20> mFECmaxID;                                            ///< FEC ID  max number of hit channels
  std::unordered_map<EventType, TProfile2D*> mRMS;                           ///< ADC rms for EMCAL+DCAL togheter
  std::unordered_map<EventType, TProfile2D*> mMEAN;                          ///< ADC mean
  std::unordered_map<EventType, TProfile2D*> mMAX;                           ///< ADC max
  std::unordered_map<EventType, TProfile2D*> mMIN;                           ///< ADC min
  std::unordered_map<EventType, std::array<TH1*, 20>> mRawAmplitudeEMCAL;    /////< Raw amplitude in EMCAL
  std::unordered_map<EventType, std::array<TH1*, 20>> mMINRawAmplitudeEMCAL; /////< Raw amplitude in EMCAL
  std::unordered_map<EventType, std::array<TH1*, 20>> mRawAmplMaxEMCAL;      ///< Max Raw amplitude in EMCAL per cell
  std::unordered_map<EventType, std::array<TH1*, 20>> mRawAmplMinEMCAL;      ///< Min Raw amplitude in EMCAL per cell
  std::unordered_map<EventType, std::array<TProfile2D*, 20>> mRMSperSM;      ///< ADC rms per SM
  std::unordered_map<EventType, std::array<TProfile2D*, 20>> mMEANperSM;     ///< ADC mean per SM
  std::unordered_map<EventType, std::array<TProfile2D*, 20>> mMAXperSM;      ///< ADC max per SM
  std::unordered_map<EventType, std::array<TProfile2D*, 20>> mMINperSM;      ///< ADC min per SM
  std::unique_ptr<o2::emcal::MappingHandler> mMappings;                      ///< Mappings Hardware address -> Channel
  TH2F* mErrorTypeAltro = nullptr;                                           ///< Error from AltroDecoder
  TH2F* mPayloadSizePerDDL = nullptr;                                        ///< Payload size per ddl
  Int_t mNumberOfSuperpages = 0;                                             ///< Simple total superpage counter
  Int_t mNumberOfPages = 0;                                                  ///< Simple total number of superpages counter
  Int_t mNumberOfMessages = 0;
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWTASK_H
