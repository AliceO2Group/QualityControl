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
/// \file   Utilities.h
/// \author Milosz Filus
// Custom class used to store values in ttree which will be published to ccdb as monitoring object
// probably will be modified as you want

#ifndef __O2_QC_FT0_UTILITIES_H__
#define __O2_QC_FT0_UTILITIES_H__

#include <vector>
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "Rtypes.h"
#include "TObject.h"

namespace o2::quality_control_modules::ft0
{

struct EventWithChannelData {

  EventWithChannelData() = default;
  EventWithChannelData(int pEventID, uint16_t pBC, uint32_t pOrbit, double pTimestamp, const std::vector<o2::ft0::ChannelData>& pChannels)
    : eventID(pEventID), bc(pBC), orbit(pOrbit), timestampNS(pTimestamp), channels(pChannels) {}

  int eventID = -1;
  uint16_t bc = o2::InteractionRecord::DummyBC;
  uint32_t orbit = o2::InteractionRecord::DummyOrbit;
  double timestampNS = 0;
  std::vector<o2::ft0::ChannelData> channels;

  int getEventID() const { return eventID; }
  uint16_t getBC() const { return bc; }
  uint32_t getOrbit() const { return orbit; }
  double getTimestamp() const { return timestampNS; }
  const std::vector<o2::ft0::ChannelData>& getChannels() const { return channels; }

  ClassDefNV(EventWithChannelData, 2);
};

} // namespace o2::quality_control_modules::ft0

#endif