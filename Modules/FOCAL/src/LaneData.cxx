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
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iomanip>

using namespace o2::quality_control_modules::focal;

void LaneHandler::reset()
{
  for (auto& lane : mLaneData) {
    lane.reset();
  }
}

void LaneHandler::resetLane(std::size_t laneID)
{
  handleLaneIndex(laneID);
  mLaneData[laneID].reset();
}

LanePayload& LaneHandler::getLane(std::size_t index)
{
  handleLaneIndex(index);
  return mLaneData[index];
}

const LanePayload& LaneHandler::getLane(std::size_t index) const
{
  handleLaneIndex(index);
  return mLaneData[index];
}

void LaneHandler::handleLaneIndex(std::size_t laneIndex) const
{
  if (laneIndex >= LaneHandler::NLANES) {
    throw LaneHandler::LaneIndexException(laneIndex);
  }
}

void LanePayload::reset()
{
  mPayload.clear();
}

void LanePayload::append(gsl::span<const uint8_t> payloadwords)
{
  std::copy(payloadwords.begin(), payloadwords.end(), std::back_inserter(mPayload));
}

void LanePayload::append(uint8_t word)
{
  mPayload.emplace_back(word);
}

gsl::span<const uint8_t> LanePayload::getPayload() const
{
  return mPayload;
}

void LanePayload::print(std::ostream& stream) const
{
  stream << "Next lane with " << mPayload.size() << " words:";
  for (const auto& word : mPayload) {
    stream << " 0x" << std::hex << static_cast<int>(word) << std::dec;
  }
}

void LaneHandler::LaneIndexException::print(std::ostream& stream) const
{
  stream << mMessage;
}

std::ostream& o2::quality_control_modules::focal::operator<<(std::ostream& stream, const LaneHandler::LaneIndexException& except)
{
  except.print(stream);
  return stream;
}

std::ostream& o2::quality_control_modules::focal::operator<<(std::ostream& stream, const LanePayload& payload)
{
  payload.print(stream);
  return stream;
}