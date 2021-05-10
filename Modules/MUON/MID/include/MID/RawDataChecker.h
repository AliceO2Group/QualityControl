// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MUON/MID/RawDataChecker.h
/// \brief  Class to check the raw data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \author Guillaume Taillepied
/// \date   9 December 2019
#ifndef O2_MID_RAWDATACHECKER_H
#define O2_MID_RAWDATACHECKER_H

#include <array>
#include <string>
#include <gsl/gsl>
#include "DataFormatsMID/ROBoard.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MID/GBTRawDataChecker.h"

namespace o2::quality_control_modules::mid
{
class RawDataChecker
{
 public:
  void init(const o2::mid::CrateMasks& masks);
  bool process(gsl::span<const o2::mid::ROBoard> localBoards, gsl::span<const o2::mid::ROFRecord> rofRecords, gsl::span<const o2::mid::ROFRecord> pageRecords);
  /// Gets the number of processed events
  unsigned int getNEventsProcessed() const;
  /// Gets the number of faulty events
  unsigned int getNEventsFaulty() const;
  /// Gets the number of busy raised
  unsigned int getNBusyRaised() const;
  /// Gets the debug message
  std::string getDebugMessage() const { return mDebugMsg; }
  void clear();

  /// Sets the delay in the electronics
  void setElectronicsDelay(const o2::mid::ElectronicsDelay& electronicsDelay) { mElectronicsDelay = electronicsDelay; }

  void setSyncTrigger(uint32_t syncTrigger);

 private:
  std::array<o2::quality_control_modules::mid::GBTRawDataChecker, o2::mid::crateparams::sNGBTs> mCheckers{}; /// GBT raw data checker
  std::string mDebugMsg{};                                                                                   /// Debug message
  o2::mid::ElectronicsDelay mElectronicsDelay{};                                                             /// Delays in the electronics
};
} // namespace o2::quality_control_modules::mid

#endif /* O2_MID_RAWDATACHECKER_H */
