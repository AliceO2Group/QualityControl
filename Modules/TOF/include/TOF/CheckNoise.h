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
/// \file   CheckNoise.h
/// \author Nicolò Jacazio nicolo.jacazio@cern.ch
/// \brief  Checker for the noise levels obtained with the TaskRaw
///

#ifndef QC_MODULE_TOF_CHECKNOISE_H
#define QC_MODULE_TOF_CHECKNOISE_H

#include "QualityControl/CheckInterface.h"
#include "Base/MessagePad.h"
#include "TPad.h"

namespace o2::quality_control_modules::tof
{

class CheckNoise : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  CheckNoise() = default;
  /// Destructor
  ~CheckNoise() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;

 private:
  /// Messages to print on the output PAD
  MessagePad mShifterMessages;
  /// Name of the accepted MO
  static constexpr char mAcceptedName[] = "hIndexEOHitRate";
  /// Maximum rate allowed before declaring a channel noisy
  float mMaxNoiseRate = 50.f; // Hz

  /// Utility to locate the channel in the TOF
  int locatedCrate = -1;       /// Located crate corresponding to channel
  int locatedTrm = -1;         /// Located TRM corresponding to channel
  int locatedSupermodule = -1; /// Located Supermodule corresponding to channel
  int locatedLink = -1;        /// Located Link corresponding to channel
  int locatedChain = -1;       /// Located Chain corresponding to channel
  int locatedTdc = -1;         /// Located Tdc corresponding to channel
  int locatedChannel = -1;     /// Located Channel (in alternative scheme) corresponding to channel
  /// Function to locate a channel in the TOF geometry
  void locateChannel(const int channel = 69461)
  {
    locatedCrate = channel / 2400;
    locatedTrm = (channel - locatedCrate * 2400) / 240 + 3;
    locatedSupermodule = locatedCrate / 4;
    locatedLink = locatedCrate % 4;
    locatedChain = (channel - locatedCrate * 2400 - (locatedTrm - 3) * 240) / 120;
    locatedTdc = (channel - 120 * locatedChain - 240 * (locatedTrm - 3) - 2400 * locatedCrate) / 8;
    locatedChannel = channel - locatedCrate * 2400 - (locatedTrm - 3) * 240 - locatedChain * 120 - locatedTdc * 8;
  }

  ClassDefOverride(CheckNoise, 1);
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_CHECKNOISE_H
