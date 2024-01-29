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
/// \file   LHCClockPhaseReductor.cxx
/// \author Piotr Konopka
///

#include "Common/LHCClockPhaseReductor.h"

#include <DataFormatsTOF/CalibLHCphaseTOF.h>

namespace o2::quality_control_modules::common
{

void* LHCClockPhaseReductor::getBranchAddress()
{
  return &mStats;
}

const char* LHCClockPhaseReductor::getBranchLeafList()
{
  return "phase/F";
}

bool LHCClockPhaseReductor::update(ConditionRetriever& retriever)
{
  if (auto lhcPhase = retriever.retrieve<o2::dataformats::CalibLHCphaseTOF>()) {
    mStats.phase = lhcPhase->getLHCphase(0);
    return true;
  }
  return false;
}

} // namespace o2::quality_control_modules::common