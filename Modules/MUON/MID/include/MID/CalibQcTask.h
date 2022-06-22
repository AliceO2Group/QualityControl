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
/// \file   CalibQcTask.h
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDCALIBQCTASK_H
#define QC_MODULE_MID_MIDCALIBQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDBase/Mapping.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Count number of digits per detector elements

class CalibQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  CalibQcTask() = default;
  /// Destructor
  ~CalibQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  ///////////////////////////
  int noiseROF = 0;
  int deadROF = 0;
  o2::mid::Mapping mMapping; ///< Mapping

  std::shared_ptr<TH1F> mMultNoiseMT11B{ nullptr };
  std::shared_ptr<TH1F> mMultNoiseMT11NB{ nullptr };
  std::shared_ptr<TH1F> mMultNoiseMT12B{ nullptr };
  std::shared_ptr<TH1F> mMultNoiseMT12NB{ nullptr };
  std::shared_ptr<TH1F> mMultNoiseMT21B{ nullptr };
  std::shared_ptr<TH1F> mMultNoiseMT21NB{ nullptr };
  std::shared_ptr<TH1F> mMultNoiseMT22B{ nullptr };
  std::shared_ptr<TH1F> mMultNoiseMT22NB{ nullptr };

  std::shared_ptr<TH2F> mBendNoiseMap11{ nullptr };
  std::shared_ptr<TH2F> mBendNoiseMap12{ nullptr };
  std::shared_ptr<TH2F> mBendNoiseMap21{ nullptr };
  std::shared_ptr<TH2F> mBendNoiseMap22{ nullptr };
  std::shared_ptr<TH2F> mNBendNoiseMap11{ nullptr };
  std::shared_ptr<TH2F> mNBendNoiseMap12{ nullptr };
  std::shared_ptr<TH2F> mNBendNoiseMap21{ nullptr };
  std::shared_ptr<TH2F> mNBendNoiseMap22{ nullptr };

  std::shared_ptr<TH1F> mMultDeadMT11B{ nullptr };
  std::shared_ptr<TH1F> mMultDeadMT11NB{ nullptr };
  std::shared_ptr<TH1F> mMultDeadMT12B{ nullptr };
  std::shared_ptr<TH1F> mMultDeadMT12NB{ nullptr };
  std::shared_ptr<TH1F> mMultDeadMT21B{ nullptr };
  std::shared_ptr<TH1F> mMultDeadMT21NB{ nullptr };
  std::shared_ptr<TH1F> mMultDeadMT22B{ nullptr };
  std::shared_ptr<TH1F> mMultDeadMT22NB{ nullptr };

  std::shared_ptr<TH2F> mBendDeadMap11{ nullptr };
  std::shared_ptr<TH2F> mBendDeadMap12{ nullptr };
  std::shared_ptr<TH2F> mBendDeadMap21{ nullptr };
  std::shared_ptr<TH2F> mBendDeadMap22{ nullptr };
  std::shared_ptr<TH2F> mNBendDeadMap11{ nullptr };
  std::shared_ptr<TH2F> mNBendDeadMap12{ nullptr };
  std::shared_ptr<TH2F> mNBendDeadMap21{ nullptr };
  std::shared_ptr<TH2F> mNBendDeadMap22{ nullptr };
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDCALIBQCTASK_H
