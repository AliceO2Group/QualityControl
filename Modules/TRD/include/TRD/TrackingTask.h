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
/// \file   TrackingTask.h
/// \author Salman Malik
///

#ifndef QC_MODULE_TRD_TRDTRACKINGTASK_H
#define QC_MODULE_TRD_TRDTRACKINGTASK_H

#include "QualityControl/TaskInterface.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TProfile2D.h"
#include <TH1.h>
#include <Framework/TimingInfo.h>
#include "DataFormatsTRD/TrackTRD.h"

class TH1D;
class TH2D;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{
using GTrackID = o2::dataformats::GlobalTrackID;

class TrackingTask final : public TaskInterface
{
 public:
  TrackingTask() = default;
  ~TrackingTask() = default;

  void retrieveCCDBSettings();
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void buildHistograms();
  void axisConfig(TH1* h, const char* xTitle, const char* yTitle, const char* zTitle, bool stat, float xOffset = 1., float yOffset = 1.);
  void publishObject(TObject* aObject, const char* drawOpt = "", const char* dispayOpt = "");
  void drawLayers(TH2* hist);
  long int mTimestamp;

  static constexpr int NLayers = 6;                                // number of TRD layers
  TString chrg[2] = { "Pos", "Neg" };                              // charge of tracks
  TH1D* mNtracks = nullptr;                                        // number of ITS-TPC-TRD tracks per event
  TH1D* mNtracklets = nullptr;                                     // number of TRD tracklets per track
  TH1D* mTrackEta = nullptr;                                       // eta of ITS-TPC-TRD tracks
  TH1D* mTrackPhi = nullptr;                                       // phi of ITS-TPC-TRD tracks
  TH1D* mTrackPt = nullptr;                                        // pt of ITS-TPC-TRD tracks
  TH1D* mTrackChi2 = nullptr;                                      // reduced chi2 of ITS-TPC-TRD tracks
  TH1D* mDeltaY = nullptr;                                         // residual in y direction (trackposiny - trackletposiny)
  TH1D* mDeltaZ = nullptr;                                         // residual in z direction (trackposinz - trackletposinz)
  TH2D* mDeltaYDet = nullptr;                                      // residual in y direction vs. 540 detectors
  TH2D* mDeltaZDet = nullptr;                                      // residual in z direction vs. 540 detectors
  TH2D* mDeltaYsphi = nullptr;                                     // residual in y direction vs. sin(phi) track seed
  TH2D* mTrackletDef = nullptr;                                    // tracklet slope vs. tracklet position
  std::array<TProfile2D*, 2> mTrackletsEtaPhi;                     // eta vs. phi as function of tracklets
  std::array<std::array<TH2D*, 2>, NLayers> mTracksEtaPhiPerLayer; // eta vs. phi of tracks per layer
  std::array<TH2D*, NLayers> mDeltaYinEtaPerLayer;                 // residual in y direction vs. eta per layer
  std::array<TH2D*, NLayers> mDeltaYinPhiPerLayer;                 // residual in y direction vs. phi per layer
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRACKINGTASK_H
