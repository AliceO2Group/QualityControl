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
/// \file   LHCClockPhaseReductor.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_LHCCLOCKPHASEREDUCTOR_H
#define QUALITYCONTROL_LHCCLOCKPHASEREDUCTOR_H

#include "QualityControl/ReductorConditionAny.h"

namespace o2::quality_control_modules::common
{

/// \brief A Reductor which obtains the LHC clock phase.
///
/// A Reductor which obtains the LHC clock phase based on the corresponding object in CCDB.
/// It produces a branch in the format: "phase/F"
class LHCClockPhaseReductor : public quality_control::postprocessing::ReductorConditionAny
{
 public:
  LHCClockPhaseReductor() = default;
  ~LHCClockPhaseReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  bool update(ConditionRetriever& retriever) override;

 private:
  struct {
    Float_t phase;
  } mStats;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_LHCCLOCKPHASEREDUCTOR_H
