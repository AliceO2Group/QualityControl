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

//
// \file   SeparationPowerReductor.h
// \author Marcel Lesch
//
#ifndef QC_MODULE_TPC_SEPARATIONPOWERREDUCTOR_H
#define QC_MODULE_TPC_SEPARATIONPOWERREDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2::quality_control_modules::tpc
{

/// \brief Reductor for TPC PID separation power values
///
/// Reductor for TPC PID separation power values
class SeparationPowerReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  SeparationPowerReductor() = default;
  ~SeparationPowerReductor() = default;

  void* getBranchAddress() final;
  const char* getBranchLeafList() final;
  void update(TObject* obj) final;

 private:
  struct {
    float amplitudePi;
    float meanPi;
    float sigmaPi;
    float amplitudeEl;
    float meanEl;
    float sigmaEl;
    float separationPower;
    float chiSquareOverNdf;
  } mSeparationPower;
};

} // namespace o2::quality_control_modules::tpc
#endif // QC_MODULE_TPC_SEPARATIONPOWERREDUCTOR_H
