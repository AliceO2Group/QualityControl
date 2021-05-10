// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MUON/MID/RawDataChecker.cxx
/// \brief  Class to check the raw data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \author Guillaume Taillepied
/// \date   9 December 2019

#include "MID/RawDataChecker.h"

#include <unordered_map>
#include "MIDRaw/CrateParameters.h"

namespace o2::quality_control_modules::mid
{

  void RawDataChecker::init(const o2::mid::CrateMasks& crateMasks)
{
  /// Initializes the checkers
  for (uint16_t igbt = 0; igbt < o2::mid::crateparams::sNGBTs; ++igbt) {
    mCheckers[igbt].setElectronicsDelay(mElectronicsDelay);
    mCheckers[igbt].init(igbt, crateMasks.getMask(igbt));
  }
}

  bool RawDataChecker::process(gsl::span<const o2::mid::ROBoard> localBoards, gsl::span<const o2::mid::ROFRecord> rofRecords, gsl::span<const o2::mid::ROFRecord> pageRecords)
{
  /// Checks the raw data

  bool isOk = true;
  mDebugMsg.clear();
  std::unordered_map<uint16_t, std::vector<o2::mid::ROFRecord>> rofs;
  for (auto& rof : rofRecords) {
    auto& loc = localBoards[rof.firstEntry];
    auto crateId = o2::mid::raw::getCrateId(loc.boardId);
    auto linkId = o2::mid::crateparams::getGBTIdFromBoardInCrate(o2::mid::raw::getLocId(loc.boardId));
    auto feeId = o2::mid::crateparams::makeGBTUniqueId(crateId, linkId);
    rofs[feeId].emplace_back(rof);
  }

  for (auto& item : rofs) {
    isOk &= mCheckers[item.first].process(localBoards, item.second, pageRecords);
    mDebugMsg += mCheckers[item.first].getDebugMessage();
  }

  return isOk;
}

void RawDataChecker::setSyncTrigger(uint32_t syncTrigger)
{
  /// Sets the trigger use to verify if all data of an event where received
  for (auto& checker : mCheckers) {
    checker.setSyncTrigger(syncTrigger);
  }
}

unsigned int RawDataChecker::getNEventsProcessed() const
{
  /// Gets the number of processed events
  unsigned int sum = 0;
  for (auto& checker : mCheckers) {
    sum += checker.getNEventsProcessed();
  }
  return sum;
}

unsigned int RawDataChecker::getNEventsFaulty() const
{
  /// Gets the number of faulty events
  unsigned int sum = 0;
  for (auto& checker : mCheckers) {
    sum += checker.getNEventsFaulty();
  }
  return sum;
}

unsigned int RawDataChecker::getNBusyRaised() const
{
  /// Gets the number of busy raised
  unsigned int sum = 0;
  for (auto& checker : mCheckers) {
    sum += checker.getNBusyRaised();
  }
  return sum;
}

void RawDataChecker::clear()
{
  /// Clears the statistics
  for (auto& checker : mCheckers) {
    checker.clear();
  }
}

} // namespace o2::quality_control_modules::mid
