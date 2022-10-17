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
/// \file   TestbeamRawTask.h
/// \author My Name
///

#ifndef QC_MODULE_FOCAL_FOCALTESTBEAMRAWTASK_H
#define QC_MODULE_FOCAL_FOCALTESTBEAMRAWTASK_H

#include <array>

#include "QualityControl/TaskInterface.h"
#include "FOCAL/PadWord.h"
#include "FOCAL/PadDecoder.h"
#include "FOCAL/PadMapper.h"

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::focal
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class TestbeamRawTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  TestbeamRawTask() = default;
  /// Destructor
  ~TestbeamRawTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void processPadPayload(gsl::span<const PadGBTWord> gbtpayload);
  void processPadEvent(gsl::span<const PadGBTWord> gbtpayload);

  PadDecoder mPadDecoder; ///< Decoder for pad data
  PadMapper mPadMapper;   ///< Mapping for Pads

  TH1* mPayloadSizePadsGBT;                             ///< Payload size GBT words of pad data
  std::array<TH2*, PadData::NASICS> mPadASICChannelADC; ///< ADC per channel for each ASIC
  std::array<TH2*, PadData::NASICS> mPadASICChannelTOA; ///< TOA per channel for each ASIC
  std::array<TH2*, PadData::NASICS> mPadASICChannelTOT; ///< TOT per channel for each ASIC
  std::array<TH2*, PadData::NASICS> mHitMapPadASIC;     ///< Hitmap per ASIC
};

} // namespace o2::quality_control_modules::focal

#endif // QC_MODULE_FOCAL_FOCALTESTBEAMRAWTASK_H
