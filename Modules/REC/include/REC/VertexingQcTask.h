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
/// \file   VertexingQcTask.h
/// \author Chiara Zampolli
///

#ifndef QC_MODULE_REC_RECVERTEXINGQCTASK_H
#define QC_MODULE_REC_RECVERTEXINGQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "Steer/MCKinematicsReader.h"

#include "SimulationDataFormat/MCEventLabel.h"

class TH1F;
class TProfile;

#include <unordered_map>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::rec
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class VertexingQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  VertexingQcTask() = default;
  /// Destructor
  ~VertexingQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TH1F* mX = nullptr;                     // vertex X
  TH1F* mY = nullptr;                     // vertex Y
  TH1F* mZ = nullptr;                     // vertex Z
  TH1F* mNContributors = nullptr;         // vertex N contributors
  TProfile* mTimeUncVsNContrib = nullptr; // time uncertainty vs N contributors

  // MC related part
  bool mUseMC = false;
  o2::steer::MCKinematicsReader mMCReader; // MC reader
  TProfile* mPurityVsMult = nullptr;       // purity vs multiplicity

  std::unordered_map<o2::MCEventLabel, int> mMapEvIDSourceID; // unordered_map counting the number of vertices reconstructed per event and source (--> MCEventLabel)
  TH1F* mNPrimaryMCEvWithVtx = nullptr;                       // event multiplicity for MC events with at least 1 vertex
  TH1F* mNPrimaryMCGen = nullptr;                             // event multiplicity for all MC events
  TH1F* mVtxEffVsMult = nullptr;                              // for vertex efficiency
  TProfile* mCloneFactorVsMult = nullptr;                     // clone factor vs multiplicity
  TProfile* mVtxResXVsMult = nullptr;                         // vertex resolution in X
  TProfile* mVtxResYVsMult = nullptr;                         // vertex resolution in Y
  TProfile* mVtxResZVsMult = nullptr;                         // vertex resolution in Z
  TProfile* mVtxPullsXVsMult = nullptr;                       // vertex pulls in X
  TProfile* mVtxPullsYVsMult = nullptr;                       // vertex pulls in Y
  TProfile* mVtxPullsZVsMult = nullptr;                       // vertex pulls in Z
};

} // namespace o2::quality_control_modules::rec

#endif // QC_MODULE_REC_RECVERTEXINGQCTASK_H
