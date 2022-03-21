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
/// \file   ITSFeeCheck.h
/// \auhtor Liang Zhang
/// \author Jian Liu
/// \author Antonio Palasciano
///

#ifndef QC_MODULE_ITS_ITSFEECHECK_H
#define QC_MODULE_ITS_ITSFEECHECK_H

#include "QualityControl/CheckInterface.h"
#include <TH2Poly.h>

namespace o2::quality_control_modules::its
{

/// \brief  Check the FAULT flag for the lanes

class ITSFeeCheck : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  ITSFeeCheck() = default;
  /// Destructor
  ~ITSFeeCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  ClassDefOverride(ITSFeeCheck, 2);

  static constexpr int NLayer = 7;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const int NLanePerStaveLayer[NLayer] = { 9, 9, 9, 16, 16, 28, 28 };
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  static constexpr int NFlags = 3;
  const double minTextPosY[NLayer] = { 0.43, 0.41, 0.39, 0.23, 0.21, 0.16, 0.13 }; // Text y coordinates in TH2Poly
  std::string mLaneStatusFlag[NFlags] = { "WARNING", "ERROR", "FAULT" };
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFeeCheck_H
