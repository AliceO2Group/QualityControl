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
/// \file   TimeGainCalibReductor.h
/// \author Marcel Lesch
///

#ifndef QUALITYCONTROL_TIMEGAINCALIBREDUCTOR_H
#define QUALITYCONTROL_TIMEGAINCALIBREDUCTOR_H

#include "QualityControl/ReductorConditionAny.h"
#include <DataFormatsTPC/Defs.h>

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor for calibration objects of the TPC drift velocity
///
/// A Reductor for calibration objects of the TPC time gain.
/// It produces a branch in the format: "meanEntries[2][5]/F:stddevEntries[2][5]:meanGain[2][5]:diffCorrectionTgl[2][5]"
/// Format Details:
/// [2][5] charge type (Max = 0, Tot = 1) per type (IROCgem = 0, OROC1gem = 1, OROC2gem = 2, OROC3gem = 3, All Stacks = 4)

class TimeGainCalibReductor : public quality_control::postprocessing::ReductorConditionAny
{
 public:
  TimeGainCalibReductor() = default;
  ~TimeGainCalibReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  bool update(ConditionRetriever& retriever) override;

 private:
  struct {
    Float_t meanEntries[o2::tpc::CHARGETYPES][o2::tpc::GEMSTACKSPERSECTOR + 1];
    Float_t stddevEntries[o2::tpc::CHARGETYPES][o2::tpc::GEMSTACKSPERSECTOR + 1];
    Float_t meanGain[o2::tpc::CHARGETYPES][o2::tpc::GEMSTACKSPERSECTOR + 1];
    Float_t diffCorrectionTgl[o2::tpc::CHARGETYPES][o2::tpc::GEMSTACKSPERSECTOR + 1]; // diff of getCorrection() for tgl(0)-tgl(1)
  } mStats;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_TIMEGAINCALIBREDUCTOR_H
