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
  return "pressure1/F:errPressure1:pressure2:errPressure2";
}

bool AtmosPressureReductor::update(ConditionRetriever& retriever)
{
  if (auto env = retriever.retrieve<o2::grp::GRPEnvVariables>()) {
    std::vector<float> pressureValues; 

    // pressure 1
    for (const auto& [time, p] : env->mEnvVars["CavernAtmosPressure"]) {
      pressureValues.emplace_back(p);
    }
    //calcMeanAndStddev(pressureValues, mStats.pressure1, mStats.errPressure1);
    pressureValues.clear();

    // pressure 2
    for (const auto& [time, p] : env->mEnvVars["CavernAtmosPressure2"]) {
       pressureValues.emplace_back(p);
    }
    //calcMeanAndStddev(pressureValues, mStats.pressure2, mStats.errPressure2);
    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::tpc