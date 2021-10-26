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
/// \file   TriggerQcTask.h
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#ifndef QC_MODULE_FT0_FT0TRIGGERQCTASK_H
#define QC_MODULE_FT0_FT0TRIGGERQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFT0/Digit.h"

#include "TH1.h"
#include "TH2.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

/// \brief Task to verify in software triggers generated on FEE
/// \author Sebastian Bysiak sbysiak@cern.ch
class TriggerQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  TriggerQcTask() = default;
  /// Destructor
  ~TriggerQcTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  unsigned int getModeParameter(std::string, unsigned int, std::map<unsigned int, std::string>);
  int getNumericalParameter(std::string, int);

  enum TrgModeSide { kAplusC,
                     kAandC,
                     kA,
                     kC
  };
  enum TrgModeThresholdVar { kAmpl,
                             kNchannels
  };
  enum ComparisonResult { kSWonly,
                          kTCMonly,
                          kNone,
                          kBoth
  };
  std::map<int, std::string> mMapDigitTrgNames;
  std::map<int, bool> mMapTrgSoftware;
  const int mNChannelsA = 96;

  // trigger parameters:
  // - modes
  unsigned int mModeThresholdVar;
  unsigned int mModeSide;
  // - time window for vertex trigger
  int mThresholdTimeLow;
  int mThresholdTimeHigh;
  // - parameters for (Semi)Central triggers
  //   same parameters re-used for both Ampl and Nchannels thresholds
  int mThresholdCenA;
  int mThresholdCenC;
  int mThresholdCenSum;
  int mThresholdSCenA;
  int mThresholdSCenC;
  int mThresholdSCenSum;

  // Objects which will be published
  std::unique_ptr<TH1F> mHistTriggersSw;
  std::unique_ptr<TH2F> mHistTriggersSoftwareVsTCM;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_FT0TRIGGERQCTASK_H
