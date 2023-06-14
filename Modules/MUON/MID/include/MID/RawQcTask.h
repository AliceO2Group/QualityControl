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
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDRAWQCTASK_H
#define QC_MODULE_MID_MIDRAWQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "MIDQC/RawDataChecker.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MUONCommon/MergeableTH2Ratio.h"

class TH1F;
class TH2F;
class TProfile;

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
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  std::unique_ptr<o2::mid::Decoder> mDecoder{ nullptr };
  o2::mid::RawDataChecker mChecker{};
  o2::mid::FEEIdConfig mFeeIdConfig{};
  o2::mid::ElectronicsDelay mElectronicsDelay{};
  o2::mid::CrateMasks mCrateMasks{};

  void InitMultiplicity()
  {
    for (int i = 0; i < 4; i++)
      multHitB[i] = multHitNB[i] = 0;
  };
  static int Pattern(uint16_t pattern);
  // static int BPPattern(const o2::mid::ROBoard& board, uint8_t station);
  // static int NBPPattern(const o2::mid::ROBoard& board, uint8_t station);
  // void PatternMultiplicity(const o2::mid::ROBoard& board);
  void FillMultiplicity(int multB[], int multNB[]);

  ///////////////////////////
  int nEvt = 0;
  int iBC = 0;
  int iOrbit = 0;
  int nMonitor = 0;
  int nROF = 0;
  int nEntriesROF = 0;
  int nBoard = 0;
  int multHitB[4] = { 0 };
  int multHitNB[4] = { 0 };

  TH1F* mRawDataChecker{ nullptr };

  TH1F* mRawBCCounts{ nullptr };
  // TProfile* mRawBCCounts{ nullptr };

  TH2F* mRawLocalBoardsMap{ nullptr };
  TH2F* mBusyRawLocalBoards{ nullptr };
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDRAWQCTASK_H
