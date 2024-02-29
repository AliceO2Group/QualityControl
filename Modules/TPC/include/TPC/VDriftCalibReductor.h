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
/// \file   VDriftCalibReductor.h
/// \author Marcel Lesch
///

#ifndef QUALITYCONTROL_VDRIFTCALIBREDUCTOR_H
#define QUALITYCONTROL_VDRIFTCALIBREDUCTOR_H

#include "QualityControl/ReductorConditionAny.h"

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor for calibration objects of the TPC drift velocity
///
/// A Reductor for calibration objects of the TPC drift velocity.
/// It produces a branch in the format: "vdrift/F:vdrifterror"

class VDriftCalibReductor : public quality_control::postprocessing::ReductorConditionAny
{
 public:
  VDriftCalibReductor() = default;
  ~VDriftCalibReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  bool update(ConditionRetriever& retriever) override;

 private:
  struct {
    Float_t vdrift;
    Float_t vdrifterror;
  } mStats;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_VDRIFTCALIBREDUCTOR_H
