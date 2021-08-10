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
/// \author Bogdan Vulpescu
/// \author Xavier Lopez
/// \author Guillaume Taillepied

#ifndef QC_MODULE_MID_MIDRAWQCTASK_H
#define QC_MODULE_MID_MIDRAWQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "MIDQC/RawDataChecker.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Count number of digits per detector elements

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

 private:
  TH1F* mRawDataChecker = nullptr;
  TH1F* mDetElemID = nullptr;

  std::string mOutFilename;
  std::string mFeeIdConfigFilename;
  std::string mCrateMasksFilename;
  std::string mElectronicsDelaysFilename;
  bool mPerGBT;
  bool mPerFeeId;

  std::unique_ptr<o2::mid::Decoder> mDecoder{ nullptr };
  o2::mid::RawDataChecker mChecker;
  o2::mid::FEEIdConfig mFeeIdConfig;
  o2::mid::ElectronicsDelay mElectronicsDelay;
  o2::mid::CrateMasks mCrateMasks;
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDRAWQCTASK_H
