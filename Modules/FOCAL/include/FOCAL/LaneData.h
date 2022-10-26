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
#ifndef QC_MODULE_FOCAL_LANEDATA
#define QC_MODULE_FOCAL_LANEDATA

#include <array>
#include <cstdint>

namespace o2::quality_control_modules::focal
{

namespace lane_constants
{
constexpr int MAX_LANEDATA_SIZE = 0x10000;
constexpr int N_LANES = 28;

constexpr int MAX_TRIGGERS_PER_PACKET = 256;

constexpr int LENGTH_DATA = 9;
constexpr int LENGTH_IDENTIFIER = 1;
constexpr int LENGTH_PADDING = 6;
} // namespace lane_constants

struct LaneData {
  unsigned int mSize;
  unsigned int mLane;
  std::array<unsigned char, lane_constants::MAX_LANEDATA_SIZE> mLaneData;
  bool mMode;             // inner or outer barrel
  unsigned int mModuleID; // TODO: not really necessary here since module already coded in lane
  //  unsigned short half; // upper or lower
  /*
      unsigned short connector;
      unsigned short inputnumber;
      bool halfstave;
      unsigned short moduleid;
      unsigned short oblanenumber;
  */
};

struct TriggerData {
  std::array<uint64_t, lane_constants::MAX_TRIGGERS_PER_PACKET> mTrigger;
  std::array<int, lane_constants::MAX_TRIGGERS_PER_PACKET> mPosition;
  int mSize;
};

uint64_t triggerForByte(TriggerData td, int pos);
uint8_t laneId2ModuleId(uint8_t _lane);

} // namespace o2::quality_control_modules::focal

#endif // QC_MODULE_FOCAL_LANEDATA
