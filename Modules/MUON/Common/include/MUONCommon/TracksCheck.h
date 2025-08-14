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
/// \file   TracksCheck.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUON_COMMON_TRACKS_CHECK_H
#define QC_MODULE_MUON_COMMON_TRACKS_CHECK_H

#include "QualityControl/CheckInterface.h"
#include <unordered_map>

namespace o2::quality_control_modules::muon
{

/// \brief  Check the number and kinematical distribution of tracks
///
/// \author Andrea Ferrero
class TracksCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  TracksCheck() = default;
  /// Destructor
  ~TracksCheck() override = default;

  void configure() override;
  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override; private:
  std::unordered_map<std::string, Quality> mQualities;
  int mMinTracksPerTF{ 50 };
  int mMaxTracksPerTF{ 500 };
  float mMaxDeltaPhi{ 0.1 };
  float mMaxChargeAsymmetry{ 0.1 };
  float mMarkerSize{ 1.5 };

  ClassDefOverride(TracksCheck, 2);
};
} // namespace o2::quality_control_modules::muon

#endif
