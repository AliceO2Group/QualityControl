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
/// \file   AtmosPressureReductor.h
/// \author Marcel Lesch
///

#ifndef QUALITYCONTROL_ATMOSPRESSUREREDUCTOR_H
#define QUALITYCONTROL_ATMOSPRESSUREREDUCTOR_H

#include "QualityControl/ReductorConditionAny.h"

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor for atmospheric pressure
///
/// A Reductor for atmospheric pressure
/// It produces a branch in the format: "pressure1/F:errPressure1:pressure2:errPressure2"

class AtmosPressureReductor : public quality_control::postprocessing::ReductorConditionAny
{
 public:
  AtmosPressureReductor() = default;
  ~AtmosPressureReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  bool update(ConditionRetriever& retriever) override;

 private:
  struct {
    Float_t pressure1;
    Float_t errPressure1;
    Float_t pressure2;
    Float_t errPressure2;
  } mStats;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_ATMOSPRESSUREREDUCTOR_H
