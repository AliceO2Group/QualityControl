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
/// \file   CalibMQcTask.h
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDCALIBMQCTASK_H
#define QC_MODULE_MID_MIDCALIBMQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDBase/Mapping.h"

class TH1F;
class TH2F;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mid
{

/// \brief Count number of digits per detector elements

class CalibMQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  CalibMQcTask() = default;
  /// Destructor
  ~CalibMQcTask() override;

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
  int mTF = 0;
  int mNoiseROF = 0;
  int mDeadROF = 0;
  int mBadROF = 0;

  o2::mid::Mapping mMapping; ///< Mapping

  std::shared_ptr<TH1F> mMNbTimeFrame{ nullptr };
  std::shared_ptr<TH1F> mMNbNoiseROF{ nullptr };
  std::shared_ptr<TH1F> mMNbDeadROF{ nullptr };
  std::shared_ptr<TH1F> mMNbBadROF{ nullptr };

  std::shared_ptr<TH1F> mMMultNoiseMT11B{ nullptr };
  std::shared_ptr<TH1F> mMMultNoiseMT11NB{ nullptr };
  std::shared_ptr<TH1F> mMMultNoiseMT12B{ nullptr };
  std::shared_ptr<TH1F> mMMultNoiseMT12NB{ nullptr };
  std::shared_ptr<TH1F> mMMultNoiseMT21B{ nullptr };
  std::shared_ptr<TH1F> mMMultNoiseMT21NB{ nullptr };
  std::shared_ptr<TH1F> mMMultNoiseMT22B{ nullptr };
  std::shared_ptr<TH1F> mMMultNoiseMT22NB{ nullptr };

  std::shared_ptr<TProfile2D> mMBendNoiseMap11{ nullptr };
  std::shared_ptr<TProfile2D> mMBendNoiseMap12{ nullptr };
  std::shared_ptr<TProfile2D> mMBendNoiseMap21{ nullptr };
  std::shared_ptr<TProfile2D> mMBendNoiseMap22{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendNoiseMap11{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendNoiseMap12{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendNoiseMap21{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendNoiseMap22{ nullptr };

  std::shared_ptr<TH1F> mMMultDeadMT11B{ nullptr };
  std::shared_ptr<TH1F> mMMultDeadMT11NB{ nullptr };
  std::shared_ptr<TH1F> mMMultDeadMT12B{ nullptr };
  std::shared_ptr<TH1F> mMMultDeadMT12NB{ nullptr };
  std::shared_ptr<TH1F> mMMultDeadMT21B{ nullptr };
  std::shared_ptr<TH1F> mMMultDeadMT21NB{ nullptr };
  std::shared_ptr<TH1F> mMMultDeadMT22B{ nullptr };
  std::shared_ptr<TH1F> mMMultDeadMT22NB{ nullptr };

  std::shared_ptr<TProfile2D> mMBendDeadMap11{ nullptr };
  std::shared_ptr<TProfile2D> mMBendDeadMap12{ nullptr };
  std::shared_ptr<TProfile2D> mMBendDeadMap21{ nullptr };
  std::shared_ptr<TProfile2D> mMBendDeadMap22{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendDeadMap11{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendDeadMap12{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendDeadMap21{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendDeadMap22{ nullptr };

  std::shared_ptr<TH1F> mMMultBadMT11B{ nullptr };
  std::shared_ptr<TH1F> mMMultBadMT11NB{ nullptr };
  std::shared_ptr<TH1F> mMMultBadMT12B{ nullptr };
  std::shared_ptr<TH1F> mMMultBadMT12NB{ nullptr };
  std::shared_ptr<TH1F> mMMultBadMT21B{ nullptr };
  std::shared_ptr<TH1F> mMMultBadMT21NB{ nullptr };
  std::shared_ptr<TH1F> mMMultBadMT22B{ nullptr };
  std::shared_ptr<TH1F> mMMultBadMT22NB{ nullptr };

  std::shared_ptr<TProfile2D> mMBendBadMap11{ nullptr };
  std::shared_ptr<TProfile2D> mMBendBadMap12{ nullptr };
  std::shared_ptr<TProfile2D> mMBendBadMap21{ nullptr };
  std::shared_ptr<TProfile2D> mMBendBadMap22{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendBadMap11{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendBadMap12{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendBadMap21{ nullptr };
  std::shared_ptr<TProfile2D> mMNBendBadMap22{ nullptr };
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDCALIBMQCTASK_H
