// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ChannelTimeCalibrationCheck.h
/// \author Milosz Filus

#ifndef QC_MODULE_FT0_FT0CalibrationCheck_H
#define QC_MODULE_FT0_FT0CalibrationCheck_H

// Quality Control
#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::ft0
{

/// \brief
///
class ChannelTimeCalibrationCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ChannelTimeCalibrationCheck() = default;
  /// Destructor
  ~ChannelTimeCalibrationCheck() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult) override;
  ClassDefOverride(ChannelTimeCalibrationCheck, 1);

 private:
  static constexpr const char* MEAN_WARNING_KEY = "MeanWarning";
  static constexpr const char* MEAN_ERROR_KEY = "MeanError";
  static constexpr const char* RMS_WARNING_KEY = "RMSWarning";
  static constexpr const char* RMS_ERROR_KEY = "RMSError";
  static constexpr const char* MIN_ENTRIES_KEY = "MinEntries";

 private:
  double mMeanWarning;
  double mMeanError;
  double mRMSWarning;
  double mRMSError;
  int mMinEntries;
};

} // namespace o2::quality_control_modules::ft0

#endif
