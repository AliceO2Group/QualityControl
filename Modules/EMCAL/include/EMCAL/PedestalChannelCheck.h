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
/// \file   PedestalChannelCheck.h
/// \author Sierra Cantway
///

#ifndef QC_MODULE_EMCAL_EMCALPEDESTALCHANNELCHECK_H
#define QC_MODULE_EMCAL_EMCALPEDESTALCHANNELCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::emcal
{

/// \brief  Check whether a plot is good or not.
///
/// \author Sierra Cantway
class PedestalChannelCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  PedestalChannelCheck() = default;
  /// Destructor
  ~PedestalChannelCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;

 private:
  /************************************************
   * threshold cuts                               *
   ************************************************/

  float mBadPedestalChannelADCRangeLow = 20.;       ///< Bad Low Threshold for mean ADC used in the Pedestal Channels check
  float mBadPedestalChannelADCRangeHigh = 100.;     ///< Bad High Threshold for mean ADC used in the Pedestal Channels check
  float mBadPedestalChannelOverMeanThreshold = 0.2; ///< Bad Threshold used in the time Pedestal Channels points outside of mean check

  float mMedPedestalChannelADCRangeLow = 40.;       ///< Med Low Threshold for mean ADC used in the Pedestal Channels check
  float mMedPedestalChannelADCRangeHigh = 80.;      ///< Med High Threshold for mean ADC used in the Pedestal Channels check
  float mMedPedestalChannelOverMeanThreshold = 0.1; ///< Med Threshold used in the time Pedestal Channels points outside of mean check

  float mSigmaPedestalChannel = 2.; ///< Number of sigmas used in the Pedestal Channel points outside of mean check

  ClassDefOverride(PedestalChannelCheck, 1);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALPEDESTALCHANNELCHECK_H
