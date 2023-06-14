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
/// \file   QcMFTReadoutTask.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

#ifndef QC_MFT_READOUT_TASK_H
#define QC_MFT_READOUT_TASK_H

// O2
#include <ITSMFTReconstruction/ChipMappingMFT.h>

// Quality Control
#include "QualityControl/TaskInterface.h"
#include "Headers/RAWDataHeader.h"
#include "Headers/RDHAny.h"
#include "DetectorsRaw/RDHUtils.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mft
{

/// \brief MFT Basic Readout Header QC task
///
class QcMFTReadoutTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
  // addapted from ITSFeeTask
  struct MFTDDW { // GBT diagnostic word
    union {
      uint64_t word0 = 0x0;
      struct {
        uint64_t laneStatus : 50;
        uint16_t someInfo : 14;
      } laneBits;
    } laneWord;
    union {
      uint64_t word1 = 0x0;
      struct {
        uint16_t someInfo2 : 8;
        uint16_t id : 8;
        uint64_t padding : 48;
      } indexBits;
    } indexWord;
  };

 public:
  /// \brief Constructor
  QcMFTReadoutTask() = default;
  /// Destructor
  ~QcMFTReadoutTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  const int nLanes = 25;
  const int maxRUidx = 104;
  std::array<int, (104 * 25)> mChipIndex;

  int mHalf[936] = { 0 };
  int mDisk[936] = { 0 };
  int mFace[936] = { 0 };
  int mZone[936] = { 0 };
  int mSensor[936] = { 0 };
  int mTransID[936] = { 0 };
  int mLayer[936] = { 0 };
  int mLadder[936] = { 0 };
  float mX[936] = { 0 };
  float mY[936] = { 0 };

  // histos
  std::unique_ptr<TH1F> mRDHSummary = nullptr;
  std::unique_ptr<TH1F> mDDWSummary = nullptr;
  std::unique_ptr<TH1F> mSummaryChipOk = nullptr;
  std::unique_ptr<TH1F> mSummaryChipWarning = nullptr;
  std::unique_ptr<TH1F> mSummaryChipError = nullptr;
  std::unique_ptr<TH1F> mSummaryChipFault = nullptr;
  std::unique_ptr<TH2F> mZoneSummaryChipWarning = nullptr;
  std::unique_ptr<TH2F> mZoneSummaryChipError = nullptr;
  std::unique_ptr<TH2F> mZoneSummaryChipFault = nullptr;

  // maps RU+lane to Chip
  void generateChipIndex();

  // chip map data for summary histogram per zone
  void getChipMapData();
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_READOUT_TASK_H
