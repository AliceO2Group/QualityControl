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
/// \file   DCSPTempReductor.h
/// \author Marcel Lesch
///

#ifndef QUALITYCONTROL_DCSPTEMPREDUCTOR_H
#define QUALITYCONTROL_DCSPTEMPREDUCTOR_H

#include "QualityControl/ReductorConditionAny.h"
#include <DataFormatsTPC/Defs.h>

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor for calibration objects of the TPC DCS temperatures
///
/// A Reductor for TPC DCS temperatures.
/// It produces a branch in the format:
/// "tempSensor[18]/F:tempSensorErr[18]:tempMeanPerSide[2]:tempMeanPerSideErr[2]:tempGradXPerSide[2]:tempGradXPerSideErr[2]:tempGradYPerSide[2]:tempGradYPerSideErr[2]"
/// tempSensor[i] is the raw sensor temperature for each of the 18 sensores
/// tempMeanPerSide[i] is the mean temperature per TPC-Side (0: A-Side, 1: C-Side)
/// tempGradXPerSide[i] is the temperature gradient in x direction per TPC-Side (0: A-Side, 1: C-Side)
/// tempGradYPerSide[i] is the temperature gradient in y direction per TPC-Side (0: A-Side, 1: C-Side)

class DCSPTempReductor : public quality_control::postprocessing::ReductorConditionAny
{
 public:
  DCSPTempReductor() = default;
  ~DCSPTempReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  bool update(ConditionRetriever& retriever) override;

 private:
  struct {
    Float_t tempSensor[18];
    Float_t tempSensorErr[18]; // uncertainties

    Float_t tempMeanPerSide[2];
    Float_t tempMeanPerSideErr[2]; // uncertainties

    Float_t tempGradXPerSide[2];
    Float_t tempGradXPerSideErr[2]; // uncertainties

    Float_t tempGradYPerSide[2];
    Float_t tempGradYPerSideErr[2]; // uncertainties
  } mStats;

  void calcMeanAndStddev(std::vector<float>& values, float& mean, float& stddev);
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_DCSPTEMPREDUCTOR_H
