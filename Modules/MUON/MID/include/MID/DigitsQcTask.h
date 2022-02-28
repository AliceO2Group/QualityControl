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

#ifndef QC_MODULE_MID_MIDDIGITSQCTASK_H
#define QC_MODULE_MID_MIDDIGITSQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "MIDQC/RawDataChecker.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MID/MergeableTH2Ratio.h"

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
  TH2F* mHitsMapB{ nullptr };
  TH2F* mHitsMapNB{ nullptr };
  TH2F* mOrbitsMapB{ nullptr };
  TH2F* mOrbitsMapNB{ nullptr };
  MergeableTH2Ratio* mOccupancyMapB{ nullptr };
  MergeableTH2Ratio* mOccupancyMapNB{ nullptr };

  TH1F* mROFSizeB{ nullptr };
  TH1F* mROFSizeNB{ nullptr };

  TH2F* mROFTimeDiff{ nullptr };

  ///////////////////////////
  int nROF = 0;
  TH1F* mMultHitMT11B{ nullptr };
  TH1F* mMultHitMT11NB{ nullptr };
  TH1F* mMultHitMT12B{ nullptr };
  TH1F* mMultHitMT12NB{ nullptr };
  TH1F* mMultHitMT21B{ nullptr };
  TH1F* mMultHitMT21NB{ nullptr };
  TH1F* mMultHitMT22B{ nullptr };
  TH1F* mMultHitMT22NB{ nullptr };

  TH2F* mLocalBoardsMap{ nullptr };
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDDIGITSQCTASK_H
