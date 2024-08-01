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
/// \file   ITSChipStatusCheck.h
/// \auhtor Zhen Zhang
///

#ifndef QC_MODULE_ITS_ITSCHIPSTATUSCHECK_H
#define QC_MODULE_ITS_ITSCHIPSTATUSCHECK_H

#include "QualityControl/CheckInterface.h"
#include <TLatex.h>
#include "ITS/ITSHelpers.h"
#include <TH2Poly.h>

class TH1;
class TH2;

namespace o2::quality_control_modules::its
{

/// \brief  Check the FAULT flag for the lanes

class ITSChipStatusCheck : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  ITSChipStatusCheck() = default;
  /// Destructor
  ~ITSChipStatusCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::vector<TString> vBadStaves;

 private:
  int nCycle = 0;
  // set timer
  int TIME = 1;
  ClassDefOverride(ITSChipStatusCheck, 1);
  static const int NLayer = 7;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  std::shared_ptr<TLatex> tInfo;
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSChipStatusCheck_H
