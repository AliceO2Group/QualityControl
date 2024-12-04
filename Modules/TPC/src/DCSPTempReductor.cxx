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
/// \file   DCSPTempReductor.cxx
/// \author Marcel Lesch
///

#include "TPC/DCSPTempReductor.h"
#include "DataFormatsTPC/DCS.h"
#include "TPC/Utility.h"

namespace o2::quality_control_modules::tpc
{

void* DCSPTempReductor::getBranchAddress()
{
  return &mStats;
}

const char* DCSPTempReductor::getBranchLeafList()
{
  return "tempSensor[18]/F:tempSensorErr[18]:tempMeanPerSide[2]:tempMeanPerSideErr[2]:tempGradXPerSide[2]:tempGradXPerSideErr[2]:tempGradYPerSide[2]:tempGradYPerSideErr[2]";
}

bool DCSPTempReductor::update(ConditionRetriever& retriever)
{
  if (auto dcstemp = retriever.retrieve<o2::tpc::dcs::Temperature>()) {

    int sensorCounter = 0;
    std::vector<float> sensorData[18];
    for (const auto& sensor : dcstemp->raw) {
      for (const auto& value : sensor.data) {
        sensorData[sensorCounter].emplace_back(value.value);
      }
      calcMeanAndStddev(sensorData[sensorCounter], mStats.tempSensor[sensorCounter], mStats.tempSensorErr[sensorCounter]);
      sensorCounter++;
      if (sensorCounter > 17)
        break;
    }

    std::vector<float> sideData[3]; // 0 mean, 1 gradX, 2 gradY

    // A-Side
    for (const auto& value : dcstemp->statsA.data) {
      sideData[0].emplace_back(value.value.mean);
      sideData[1].emplace_back(value.value.gradX);
      sideData[2].emplace_back(value.value.gradY);
    }

    calcMeanAndStddev(sideData[0], mStats.tempMeanPerSide[0], mStats.tempMeanPerSideErr[0]);
    calcMeanAndStddev(sideData[1], mStats.tempGradXPerSide[0], mStats.tempGradXPerSideErr[0]);
    calcMeanAndStddev(sideData[2], mStats.tempGradYPerSide[0], mStats.tempGradYPerSideErr[0]);

    for (int iCount = 0; iCount < 3; iCount++) {
      sideData[iCount].clear();
    }

    // C-Side
    for (const auto& value : dcstemp->statsC.data) {
      sideData[0].emplace_back(value.value.mean);
      sideData[1].emplace_back(value.value.gradX);
      sideData[2].emplace_back(value.value.gradY);
    }

    calcMeanAndStddev(sideData[0], mStats.tempMeanPerSide[1], mStats.tempMeanPerSideErr[1]);
    calcMeanAndStddev(sideData[1], mStats.tempGradXPerSide[1], mStats.tempGradXPerSideErr[1]);
    calcMeanAndStddev(sideData[2], mStats.tempGradYPerSide[1], mStats.tempGradYPerSideErr[1]);

    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::tpc