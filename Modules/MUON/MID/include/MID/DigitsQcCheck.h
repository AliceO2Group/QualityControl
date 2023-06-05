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
/// \file   DigitsQcCheck.h
/// \author Bogdan Vulpescu
/// \author Valerie Ramillien

#ifndef QC_MODULE_MID_MIDDIGITSQCCHECK_H
#define QC_MODULE_MID_MIDDIGITSQCCHECK_H

#include "QualityControl/CheckInterface.h"

#include <unordered_map>
#include "QualityControl/Quality.h"
#include "MID/HistoHelper.h"

namespace o2::quality_control_modules::mid
{

/// \brief  Count number of digits per detector elements

class DigitsQcCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  DigitsQcCheck() = default;
  /// Destructor
  ~DigitsQcCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  double mMeanMultThreshold = 10.;   ///! Upper threshold on mean multiplicity
  double mMinMultThreshold = 0.001;  ///! Lower threshold on mean multiplicity
  double mLocalBoardScale = 100.;    ///! Local board scale in kHz
  int mNbEmptyLocalBoard = 117;      ///! Maximum number of allowed empty boards
  double mLocalBoardThreshold = 400; ///! Threshold on board multiplicity (kHz)
  int mNbBadLocalBoard = 10;         ///! Maximum number of local boards above threshold

  std::unordered_map<std::string, Quality> mQualityMap; ///! Quality map

  HistoHelper mHistoHelper; ///! Histogram helper

  ClassDefOverride(DigitsQcCheck, 3);
};

} // namespace o2::quality_control_modules::mid

#endif // QC_MODULE_MID_MIDDIGITSQCCHECK_H
