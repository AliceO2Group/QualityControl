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
/// \file   TrackletsTask.h
///

#ifndef QC_MODULE_TRD_TRDTRACKLETSTASK_H
#define QC_MODULE_TRD_TRDTRACKLETSTASK_H

#include "QualityControl/TaskInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "DataFormatsTRD/NoiseCalibration.h"
#include "TRDQC/StatusHelper.h"

class TH1F;
class TH2F;
class TCanvas;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

class TrackletsTask final : public TaskInterface
{
 public:
  TrackletsTask() = default;
  ~TrackletsTask() override = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  void buildHistograms();
  void drawLinesMCM(TH2F* histo);
  void drawTrdLayersGrid(TH2F* hist);
  void buildTrackletLayers();
  void drawHashedOnHistsPerLayer(int layer); //, int hcid, int rowstart, int rowend);
  void drawHashOnLayers(int layer, int hcid, int rowstart, int rowend);

  // Auxiliary functions
  bool isChamberMasked(int chamberId, const std::array<int, o2::trd::constants::MAXCHAMBER>* ptrChamber) {
    // List here the chamber status to be masked
    int BadStatus[] = {1};
    int ChamberStatus = (*ptrChamber)[chamberId];
    return (std::find(std::begin(BadStatus), std::end(BadStatus), ChamberStatus) != std::end(BadStatus));
  }

 private:
  // settings
  bool mRemoveNoise{ false };
  // histograms
  std::array<TH1F*, 3> mTrackletQ;
  TH1F* mTrackletSlope = nullptr;
  TH1F* mTrackletHCID = nullptr;
  TH1F* mTrackletPosition = nullptr;
  TH1F* mTrackletsPerEvent = nullptr;
  TH1F* mTrackletsPerEventPP = nullptr;
  TH1F* mTrackletsPerEventPbPb = nullptr;
  TH2F* mTrackletsPerHC2D = nullptr;
  TH1F* mTrackletsPerTimeFrame = nullptr;
  TH1F* mTriggersPerTimeFrame = nullptr;
  std::array<TH2F*, 6> mLayers;

  // data to pull from CCDB
  const o2::trd::NoiseStatusMCM* mNoiseMap = nullptr;
  const std::array<int, o2::trd::constants::MAXCHAMBER>* mChamberStatus = nullptr;
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRACKLETSTASK_H
