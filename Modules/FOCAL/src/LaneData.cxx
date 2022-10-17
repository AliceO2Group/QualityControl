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
#include "FOCAL/LaneData.h"
#include <cstdint>
using namespace o2::quality_control_modules::focal;

uint64_t triggerForByte(TriggerData td, int pos)
{
  for (int i = 0; i < lane_constants::MAX_TRIGGERS_PER_PACKET; i++) {
    if (td.mPosition[i] >= pos) {
      return td.mTrigger[i];
    }
  }
  return 0;
}
uint8_t laneId2ModuleId(uint8_t _lane)
{
  uint8_t mModuleid;
  return mModuleid;
}
