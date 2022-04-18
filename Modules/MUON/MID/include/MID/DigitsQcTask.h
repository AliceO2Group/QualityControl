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
/// \file   DigitsQcTask.h
/// \author Bogdan Vulpescu
/// \author Xavier Lopez
/// \author Guillaume Taillepied
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDDIGITSQCTASK_H
#define QC_MODULE_MID_MIDDIGITSQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MIDQC/RawDataChecker.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MUONCommon/MergeableTH2Ratio.h"
#include "MIDBase/Mapping.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Count number of digits per detector elements

class DigitsQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  DigitsQcTask() = default;
  /// Destructor
  ~DigitsQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  // TH2F* mHitsMapB;
  std::shared_ptr<TH2F> mHitsMapB{ nullptr };
  std::shared_ptr<TH2F> mHitsMapNB{ nullptr };
  std::shared_ptr<TH2F> mOrbitsMapB{ nullptr };
  std::shared_ptr<TH2F> mOrbitsMapNB{ nullptr };

  std::shared_ptr<TH1F> mROFSizeB{ nullptr };
  std::shared_ptr<TH1F> mROFSizeNB{ nullptr };

  std::shared_ptr<TH2F> mROFTimeDiff{ nullptr };

  ///////////////////////////
  int nROF = 0;
  o2::mid::Mapping mMapping; ///< Mapping

  std::shared_ptr<TH1F> mMultHitMT11B{ nullptr };
  std::shared_ptr<TH1F> mMultHitMT11NB{ nullptr };
  std::shared_ptr<TH1F> mMultHitMT12B{ nullptr };
  std::shared_ptr<TH1F> mMultHitMT12NB{ nullptr };
  std::shared_ptr<TH1F> mMultHitMT21B{ nullptr };
  std::shared_ptr<TH1F> mMultHitMT21NB{ nullptr };
  std::shared_ptr<TH1F> mMultHitMT22B{ nullptr };
  std::shared_ptr<TH1F> mMultHitMT22NB{ nullptr };

  std::shared_ptr<TH2F> mLocalBoardsMap{ nullptr };
  std::shared_ptr<TH2F> mLocalBoardsMap11{ nullptr };
  std::shared_ptr<TH2F> mLocalBoardsMap12{ nullptr };
  std::shared_ptr<TH2F> mLocalBoardsMap21{ nullptr };
  std::shared_ptr<TH2F> mLocalBoardsMap22{ nullptr };

  std::shared_ptr<TH2F> mBendHitsMap11{ nullptr };
  std::shared_ptr<TH2F> mBendHitsMap12{ nullptr };
  std::shared_ptr<TH2F> mBendHitsMap21{ nullptr };
  std::shared_ptr<TH2F> mBendHitsMap22{ nullptr };

  std::shared_ptr<TH2F> mNBendHitsMap11{ nullptr };
  std::shared_ptr<TH2F> mNBendHitsMap12{ nullptr };
  std::shared_ptr<TH2F> mNBendHitsMap21{ nullptr };
  std::shared_ptr<TH2F> mNBendHitsMap22{ nullptr };
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDDIGITSQCTASK_H
