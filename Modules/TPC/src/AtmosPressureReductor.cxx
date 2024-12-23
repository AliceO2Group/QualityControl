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
/// \file   AtmosPressureReductor.cxx
/// \author Marcel Lesch
///

#include "TPC/AtmosPressureReductor.h"
#include "GRPCalibration/GRPDCSDPsProcessor.h"
#include "TPC/Utility.h"

namespace o2::quality_control_modules::tpc
{

void* AtmosPressureReductor::getBranchAddress()
{
  return &mStats;
}

const char* AtmosPressureReductor::getBranchLeafList()
{
  return "cavernPressure1/F:errCavernPressure1:cavernPressure2:errCavernPressure2:surfacePressure:errSurfacePressure";
}

bool AtmosPressureReductor::update(ConditionRetriever& retriever)
{
  if (auto env = retriever.retrieve<o2::grp::GRPEnvVariables>()) {
    std::vector<float> pressureValues;

    // Cavern pressure 1
    for ([[maybe_unused]] const auto& [time, p] : env->mEnvVars["CavernAtmosPressure"]) {
      pressureValues.emplace_back((float)p);
    }
    calcMeanAndStddev(pressureValues, mStats.cavernPressure1, mStats.errCavernPressure1);
    pressureValues.clear();

    // Cavern pressure 2
    for ([[maybe_unused]] const auto& [time, p] : env->mEnvVars["CavernAtmosPressure2"]) {
      pressureValues.emplace_back((float)p);
    }
    calcMeanAndStddev(pressureValues, mStats.cavernPressure2, mStats.errCavernPressure2);
    pressureValues.clear();

    // Surface pressure
    for ([[maybe_unused]] const auto& [time, p] : env->mEnvVars["SurfaceAtmosPressure"]) {
      pressureValues.emplace_back((float)p);
    }
    calcMeanAndStddev(pressureValues, mStats.surfacePressure, mStats.errSurfacePressure);
    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::tpc