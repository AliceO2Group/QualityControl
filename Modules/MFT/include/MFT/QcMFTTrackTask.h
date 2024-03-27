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
/// \file   QcMFTTrackTask.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Diana Maria Krupova
/// \author Katarina Krizkova Gajdosova
/// \author David Grund
///

#ifndef QC_MFT_TRACK_TASK_H
#define QC_MFT_TRACK_TASK_H

// ROOT
#include <TH1.h>
#include <TH2.h>
// Quality Control
#include "QualityControl/TaskInterface.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
// O2
#include "CommonConstants/LHCConstants.h"
#include "MFTBase/GeometryTGeo.h"

using namespace o2::quality_control::core;
using namespace o2::quality_control_modules::common;
using namespace std;

namespace o2::quality_control_modules::mft
{

/// \brief MFT Track QC task
///
class QcMFTTrackTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  QcMFTTrackTask() = default;
  /// Destructor
  ~QcMFTTrackTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  o2::mft::GeometryTGeo* mGeom = nullptr;

  std::unique_ptr<TH1FRatio> mNumberOfTracksPerTF = nullptr;
  std::unique_ptr<TH1FRatio> mTrackNumberOfClusters = nullptr;
  std::unique_ptr<TH1FRatio> mCATrackNumberOfClusters = nullptr;
  std::unique_ptr<TH1FRatio> mLTFTrackNumberOfClusters = nullptr;
  std::unique_ptr<TH1FRatio> mTrackInvQPt = nullptr;
  std::unique_ptr<TH1FRatio> mTrackChi2 = nullptr;
  std::unique_ptr<TH1FRatio> mTrackCharge = nullptr;
  std::unique_ptr<TH1FRatio> mTrackPhi = nullptr;
  std::unique_ptr<TH1FRatio> mPositiveTrackPhi = nullptr;
  std::unique_ptr<TH1FRatio> mNegativeTrackPhi = nullptr;
  std::unique_ptr<TH1FRatio> mTrackEta = nullptr;
  std::array<unique_ptr<TH1FRatio>, 6> mTrackEtaNCls = { nullptr };
  std::array<unique_ptr<TH1FRatio>, 6> mTrackPhiNCls = { nullptr };
  std::array<unique_ptr<TH2FRatio>, 6> mTrackXYNCls = { nullptr };
  std::array<unique_ptr<TH2FRatio>, 6> mTrackEtaPhiNCls = { nullptr };
  std::unique_ptr<TH1FRatio> mCATrackEta = nullptr;
  std::unique_ptr<TH1FRatio> mLTFTrackEta = nullptr;
  std::unique_ptr<TH1FRatio> mCATrackPt = nullptr;
  std::unique_ptr<TH1FRatio> mLTFTrackPt = nullptr;
  std::unique_ptr<TH1FRatio> mTrackTanl = nullptr;
  std::unique_ptr<TH1FRatio> mTrackROFNEntries = nullptr;
  std::unique_ptr<TH1FRatio> mTracksBC = nullptr;
  std::unique_ptr<TH1FRatio> mAssociatedClusterFraction = nullptr;
  std::unique_ptr<TH2FRatio> mClusterRatioVsBunchCrossing = nullptr;

  static constexpr array<short, 6> sMinNClustersList = { 5, 6, 7, 8, 9, 10 };
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_TRACK_TASK_H
