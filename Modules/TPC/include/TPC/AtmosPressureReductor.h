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

/// \brief A reductor for atmospheric pressure
///
/// A reductor for atmospheric pressure
/// It produces a branch in the format: "cavernPressure1/F:errCavernPressure1:cavernPressure2:errCavernPressure2:surfacePressure:errSurfacePressure"

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
    Float_t cavernPressure1;
    Float_t errCavernPressure1;
    Float_t cavernPressure2;
    Float_t errCavernPressure2;
    Float_t surfacePressure;
    Float_t errSurfacePressure;
  } mStats;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_ATMOSPRESSUREREDUCTOR_H
