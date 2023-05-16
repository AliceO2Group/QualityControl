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
/// \file   HelperFIT.cxx
/// \author Artur Furs afurs@cern.ch

#include "FITCommon/HelperFIT.h"

namespace o2::quality_control_modules::fit
{
const std::map<unsigned int, std::string> HelperTrgFIT::sMapTrgBits = {
  { o2::fit::Triggers::bitA, "OrA" },
  { o2::fit::Triggers::bitC, "OrC" },
  { o2::fit::Triggers::bitVertex, "Vertex" },
  { o2::fit::Triggers::bitCen, "Central" },
  { o2::fit::Triggers::bitSCen, "SemiCentral" },
  { o2::fit::Triggers::bitLaser, "Laser" },
  { o2::fit::Triggers::bitOutputsAreBlocked, "OutputsAreBlocked" },
  { o2::fit::Triggers::bitDataIsValid, "DataIsValid" }
};
const std::map<unsigned int, std::string> HelperTrgFIT::sMapBasicTrgBitsFDD = {
  { o2::fit::Triggers::bitA, "OrA" },
  { o2::fit::Triggers::bitC, "OrC" },
  { o2::fit::Triggers::bitVertex, "Vertex" },
  { o2::fit::Triggers::bitCen, "Central" },
  { o2::fit::Triggers::bitSCen, "SemiCentral" }
};

const std::map<unsigned int, std::string> HelperTrgFIT::sMapBasicTrgBitsFT0 = {
  { o2::fit::Triggers::bitA, "OrA" },
  { o2::fit::Triggers::bitC, "OrC" },
  { o2::fit::Triggers::bitVertex, "Vertex" },
  { o2::fit::Triggers::bitCen, "Central" },
  { o2::fit::Triggers::bitSCen, "SemiCentral" }
};

const std::map<unsigned int, std::string> HelperTrgFIT::sMapBasicTrgBitsFV0 = {
  { o2::fit::Triggers::bitA, "OrA" },
  { o2::fit::Triggers::bitAOut, "OrAOut" },
  { o2::fit::Triggers::bitTrgNchan, "TrgNChan" },
  { o2::fit::Triggers::bitTrgCharge, "TrgCharge" },
  { o2::fit::Triggers::bitAIn, "OrAIn" }
};
const std::array<std::vector<uint8_t>, 256> HelperTrgFIT::sArrDecomposed1Byte = HelperTrgFIT::decompose1Byte();
} // namespace o2::quality_control_modules::fit
