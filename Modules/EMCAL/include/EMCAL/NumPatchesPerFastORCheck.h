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
/// \file   NumPatchesPerFastORCheck.h
/// \author Sierra Cantway
///

#ifndef QC_MODULE_EMCAL_EMCALNUMPATCHESPERFASTORCHECK_H
#define QC_MODULE_EMCAL_EMCALNUMPATCHESPERFASTORCHECK_H

#include "QualityControl/CheckInterface.h"
#include "EMCALBase/TriggerMappingV2.h"
#include "EMCALBase/Geometry.h"
#include <set>
#include <functional>

namespace o2::quality_control_modules::emcal
{

/// \brief  Check whether a plot is good or not.
///
/// \author Sierra Cantway
class NumPatchesPerFastORCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  NumPatchesPerFastORCheck() = default;
  /// Destructor
  ~NumPatchesPerFastORCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;  struct FastORNoiseInfo {
    int mCounts;
    int mTRUIndex;
    int mFastORIndex;
    int mPosPhi;
    int mPosEta;

    bool operator==(const FastORNoiseInfo& other) const
    {
      return mCounts == other.mCounts && mFastORIndex == other.mFastORIndex;
    }
    bool operator<(const FastORNoiseInfo& other) const
    {
      if (mCounts < other.mCounts) {
        return true;
      }
      if (mTRUIndex == other.mTRUIndex) {
        return mFastORIndex < other.mFastORIndex;
      }
      return false;
    }
    bool operator>(const FastORNoiseInfo& other) const
    {
      if (mCounts > other.mCounts) {
        return true;
      }
      if (mTRUIndex == other.mTRUIndex) {
        return mFastORIndex > other.mFastORIndex;
      }
      return false;
    }
  };

  struct FastORNoiseLevel {
    int mCounts;
    int mFastORID;
    int mPosGlobalPhi;
    int mPosGlobalEta;
    bool mRejected;

    bool operator==(const FastORNoiseLevel& other) const
    {
      return (mCounts == other.mCounts && mFastORID == other.mFastORID);
    }

    bool operator<(const FastORNoiseLevel& other) const
    {
      if (mCounts == other.mCounts) {
        return mFastORID < other.mFastORID;
      }
      return mCounts < other.mCounts;
    }
  };

 private:
  /************************************************
   * threshold cuts                               *
   ************************************************/

  float mBadSigmaNumPatchesPerFastOR = 5.;   ///< Number of sigmas used in the Number of Patches Per FastOR fastORs outside of mean bad check
  float mMedSigmaNumPatchesPerFastOR = 999.; ///< Number of sigmas used in the Number of Patches Per FastOR fastORs outside of mean medium check
  int mLogLevelIL = 0;                       ///< Log level on InfoLogger

  o2::emcal::Geometry* mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);                                  ///< Geometry for mapping position between SM and full EMCAL
  std::unique_ptr<o2::emcal::TriggerMappingV2> mTriggerMapping = std::make_unique<o2::emcal::TriggerMappingV2>(mGeometry); ///!<! Trigger mapping
  std::set<FastORNoiseInfo, std::greater<FastORNoiseInfo>> mNoisyTRUPositions;                                             ///< Positions of all found noisy TRUs (bad quality)
  std::set<FastORNoiseInfo, std::greater<FastORNoiseInfo>> mHighCountTRUPositions;                                         ///< Positions of all FastORs with high count rate (medium Quality)

  ClassDefOverride(NumPatchesPerFastORCheck, 1);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALNUMPATCHESPERFASTORCHECK_H
