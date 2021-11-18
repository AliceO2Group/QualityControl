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
/// \file   QcMFTRecoTaskExtra.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Diana Maria Krupova
/// \author Katarina Krizkova Gajdosova

#ifndef QC_MFT_RECO_TASKEXTRA_H
#define QC_MFT_RECO_TASKEXTRA_H

// ROOT
#include <TH1.h>
#include <TH2.h>
// Quality Control
#include "QualityControl/TaskInterface.h"
// O2
#include "CommonConstants/LHCConstants.h"

using namespace o2::quality_control::core;
using namespace std;

namespace o2::quality_control_modules::mft
{

static constexpr array<short, 7> minNClustersList = { 4, 5, 6, 7, 8, 9, 10 };

/// \brief MFT Track QC task
///
class QcMFTRecoTaskExt /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  QcMFTRecoTaskExt() = default;
  /// Destructor
  ~QcMFTRecoTaskExt() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  double orbitToSeconds(uint32_t orbit, uint32_t refOrbit)
  {
    return (orbit - refOrbit) * o2::constants::lhc::LHCOrbitNS / 1E9;
  }

 private:
  unique_ptr<TH1F> mTrackNumberOfClusters = nullptr;
  unique_ptr<TH1F> mCATrackNumberOfClusters = nullptr;
  unique_ptr<TH1F> mLTFTrackNumberOfClusters = nullptr;
  unique_ptr<TH1F> mTrackOnvQPt = nullptr;
  unique_ptr<TH1F> mTrackChi2 = nullptr;
  unique_ptr<TH1F> mTrackCharge = nullptr;
  unique_ptr<TH1F> mTrackPhi = nullptr;
  unique_ptr<TH1F> mPositiveTrackPhi = nullptr;
  unique_ptr<TH1F> mNegativeTrackPhi = nullptr;
  unique_ptr<TH1F> mTrackEta = nullptr;
  array<unique_ptr<TH1F>, 7> mTrackEtaNCls = { nullptr };
  array<unique_ptr<TH1F>, 7> mTrackPhiNCls = { nullptr };
  array<unique_ptr<TH2F>, 7> mTrackXYNCls = { nullptr };
  unique_ptr<TH1F> mCATrackEta = nullptr;
  unique_ptr<TH1F> mLTFTrackEta = nullptr;
  unique_ptr<TH1F> mTrackTanl = nullptr;

  unique_ptr<TH1F> mTrackROFNEntries = nullptr;
  unique_ptr<TH1F> mClusterROFNEntries = nullptr;
  unique_ptr<TH1F> mTracksBC = nullptr;

  unique_ptr<TH1F> mNOfTracksTime = nullptr;
  unique_ptr<TH1F> mNOfClustersTime = nullptr;

  std::unique_ptr<TH1F> mClusterSensorIndex = nullptr;
  std::unique_ptr<TH1F> mClusterPatternIndex = nullptr;

  uint32_t mRefOrbit = 0; // Reference orbit used in relative time calculation
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_RECO_TASKEXTRA_H
