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

// ROOT includes
#include "TH1D.h"
#include "TH2D.h"
#include "TProfile2D.h"
#include <TH1.h>
#include <TEfficiency.h>
// O2 includes
#include "DataFormatsTRD/TrackTRD.h"
#include "DataFormatsTRD/TrackTriggerRecord.h"
#include "DataFormatsTRD/Constants.h"
#include "ReconstructionDataFormats/GlobalTrackID.h"
#include "DataFormatsGlobalTracking/RecoContainer.h"
#include <Framework/TimingInfo.h>
// QC includes
#include "QualityControl/TaskInterface.h"

class TH1D;
class TH2D;
class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

class TrackingTask final : public TaskInterface
{
 public:
  TrackingTask() = default;
  ~TrackingTask() = default;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void buildHistograms();
  void axisConfig(TH1* h, const char* xTitle, const char* yTitle, const char* zTitle, bool stat, float xOffset = 1., float yOffset = 1.);
  void publishObject(TObject* aObject, const char* drawOpt = "", const char* dispayOpt = "");
  void drawLayers(TH2* hist);
  void setEfficiency(TEfficiency* eff, TH1* hNum, TH1* hDen);
  bool mDetailedTrackQC = false;                                 // flag whether or not to expect o2::trd::TrackQC input
  std::shared_ptr<o2::globaltracking::DataRequest> mDataRequest; // specify which input to use
  o2::globaltracking::RecoContainer mRecoCont;                   // helper to acess input from reconstruction
  o2::dataformats::GlobalTrackID::mask_t mSrcSelected;           // the selected track sources from allowed ITS-TPC-TRD and TPC-TRD
  // the input data spans
  gsl::span<const o2::trd::TrackTRD> mITSTPCTRDTracks;
  gsl::span<const o2::trd::TrackTriggerRecord> mTrigITSTPCTRD;
  gsl::span<const o2::trd::TrackTRD> mTPCTRDTracks;
  gsl::span<const o2::trd::TrackTriggerRecord> mTrigTPCTRD;
  gsl::span<const o2::dataformats::TrackTPCITS> mTPCITSTracks;
  //
  float mPtMin = 0.0;                                                                 // minimum pT of tracks
  TString mChargeLabel[3] = { "Pos", "Neg", "All" };                                  // charge of tracks
  TH1D* mNtracks = nullptr;                                                           // number of ITS-TPC-TRD tracks per event
  TH1D* mNtracklets = nullptr;                                                        // number of TRD tracklets per track
  TH2D* mTrackEtaPhi = nullptr;                                                       // eta-phi distribution of ITS-TPC-TRD tracks
  TH1D* mTrackEta = nullptr;                                                          // eta of ITS-TPC-TRD tracks
  TH1D* mTrackPhi = nullptr;                                                          // phi of ITS-TPC-TRD tracks
  TH1D* mTrackPt = nullptr;                                                           // pt of ITS-TPC-TRD tracks
  TH1D* mTrackPtTPCITS = nullptr;                                                     // pt of ITS-TPC tracks
  TH1D* mTrackEtaTPCITS = nullptr;                                                    // eta of ITS-TPC tracks
  TH1D* mTrackPhiTPCITS = nullptr;                                                    // phi of ITS-TPC tracks
  TH1D* mTrackChi2 = nullptr;                                                         // reduced chi2 of ITS-TPC-TRD tracks
  TH1D* mDeltaY = nullptr;                                                            // residual in y direction (trackposiny - trackletposiny)
  TH1D* mDeltaZ = nullptr;                                                            // residual in z direction (trackposinz - trackletposinz)
  TH2D* mDeltaYDet = nullptr;                                                         // residual in y direction vs. 540 detectors
  TH2D* mDeltaZDet = nullptr;                                                         // residual in z direction vs. 540 detectors
  TH2D* mDeltaYvsSphi = nullptr;                                                      // residual in y direction vs. sin(phi) track seed
  TH2D* mTrackletDef = nullptr;                                                       // tracklet slope vs. tracklet position
  std::array<TProfile2D*, 3> mTrackletsEtaPhi;                                        // eta-phi distribution of average number of tracklets per track
  std::array<std::array<TH2D*, 2>, o2::trd::constants::NLAYER> mTracksEtaPhiPerLayer; // eta vs. phi of tracks per layer
  std::array<TH2D*, o2::trd::constants::NLAYER> mDeltaYinEtaPerLayer;                 // residual in y direction vs. eta per layer
  std::array<TH2D*, o2::trd::constants::NLAYER> mDeltaYinPhiPerLayer;                 // residual in y direction vs. phi per layer
  TEfficiency* mEfficiencyPt = nullptr;                                               // efficiency vs. pt
  TEfficiency* mEfficiencyEta = nullptr;                                              // efficiency vs. eta
  TEfficiency* mEfficiencyPhi = nullptr;                                              // efficiency vs. phi
  std::vector<double_t> trdTrigTimes = {};                                            // vector of TRD trigger times
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRACKINGTASK_H
