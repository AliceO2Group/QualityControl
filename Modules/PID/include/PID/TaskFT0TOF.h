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
/// \file   TaskFT0TOF.h
/// \author Francesca Ercolessi
/// \brief  Header task to monitor TOF PID performance
/// \since  13/01/2022
///

#ifndef QC_MODULE_PID_TASKFT0TOF_H
#define QC_MODULE_PID_TASKFT0TOF_H

#include "QualityControl/TaskInterface.h"

#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "ReconstructionDataFormats/MatchInfoTOFReco.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTRD/TrackTRD.h"
#include "TOFBase/Geo.h"
#include "DataFormatsFT0/RecPoints.h"

class TH1F;
class TH1I;
class TH2F;

namespace o2::quality_control_modules::pid
{

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;
using trkType = o2::dataformats::MatchInfoTOFReco::TrackType;

struct MyTrack {
  o2::tpc::TrackTPC trk;
  o2::dataformats::MatchInfoTOF match;
  int source = -1;
  MyTrack(const o2::dataformats::MatchInfoTOF& m, const o2::tpc::TrackTPC& t, const int s) : match(m), trk(t), source(s) {}
  MyTrack() {}
  float tofSignal() const { return match.getSignal(); }
  double tofSignalDouble() const { return match.getSignal(); }
  float tofExpSignalPi() const { return match.getLTIntegralOut().getTOF(2); }
  float tofExpSignalKa() const { return match.getLTIntegralOut().getTOF(3); }
  float tofExpSignalPr() const { return match.getLTIntegralOut().getTOF(4); }
  float tofExpSigmaPi() const { return 120; } // FIX ME
  float tofExpSigmaKa() const { return 120; } // FIX ME
  float tofExpSigmaPr() const { return 120; } // FIX ME
  float getEta() const { return trk.getEta(); }
  float getP() const { return trk.getP(); }
  float getPt() const { return trk.getPt(); }
  float getL() const
  {
    const o2::track::TrackLTIntegral& info = match.getLTIntegralOut();
    return info.getL();
  }
  const o2::tpc::TrackTPC& getTrack() { return trk; }
};

class TaskFT0TOF final : public TaskInterface
{
 public:
  enum trackType : int8_t { TPC = 0,
                            ITSTPC,
                            ITSTPCTRD,
                            TPCTRD,
                            SIZE };

  enum evTimeType : int8_t { TOF = 0,
                             FT0AC,
                             FT0A,
                             FT0C,
                             SIZEt0 };

  /// \brief Constructor
  TaskFT0TOF() = default;
  /// Destructor
  ~TaskFT0TOF() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  void processEvent(const std::vector<MyTrack>& tracks, const std::vector<o2::ft0::RecPoints>& ft0Cand);
  // track selection
  bool selectTrack(o2::tpc::TrackTPC const& track);
  void setMinPtCut(float v) { mMinPtCut = v; }
  void setEtaCut(float v) { mEtaCut = v; }
  void setMinNTPCClustersCut(float v) { mNTPCClustersCut = v; }
  void setMinDCAtoBeamPipeCut(std::array<float, 2> v)
  {
    setMinDCAtoBeamPipeCut(v[0]);
    setMinDCAtoBeamPipeYCut(v[1]);
  }
  void setMinDCAtoBeamPipeCut(float v) { mMinDCAtoBeamPipeCut = v; }
  void setMinDCAtoBeamPipeYCut(float v) { mMinDCAtoBeamPipeCutY = v; }

 private:
  std::shared_ptr<o2::globaltracking::DataRequest> mDataRequest;
  o2::globaltracking::RecoContainer mRecoCont;
  GID::mask_t mSrc = GID::getSourcesMask("ITS-TPC");
  GID::mask_t mAllowedSources = GID::getSourcesMask("TPC,TPC-TOF,ITS-TPC,ITS-TPC-TOF,TPC-TRD,TPC-TRD-TOF,ITS-TPC-TRD,ITS-TPC-TRD-TOF");
  // TPC-TOF
  gsl::span<const o2::tpc::TrackTPC> mTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mTPCTOFMatches;
  // ITS-TPC-TOF
  gsl::span<const o2::dataformats::TrackTPCITS> mITSTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mITSTPCTOFMatches;
  // TPC-TRD-TOF
  gsl::span<const o2::trd::TrackTRD> mTPCTRDTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mTPCTRDTOFMatches;
  // TPC-TRD-TOF
  gsl::span<const o2::trd::TrackTRD> mITSTPCTRDTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mITSTPCTRDTOFMatches;
  //
  std::vector<MyTrack> mMyTracks;

  // for track selection
  float mMinPtCut = 0.1f;
  float mEtaCut = 0.8f;
  int32_t mNTPCClustersCut = 40;
  float mMinDCAtoBeamPipeCut = 100.f;
  float mMinDCAtoBeamPipeCutY = 10.f;
  std::string mGRPFileName = "o2sim_grp.root";
  std::string mGeomFileName = "o2sim_geometry-aligned.root";
  float mBz = 0; ///< nominal Bz
  int mTF = -1;  // to count the number of processed TFs
  const float cinv = 33.35641;
  bool mUseFT0 = false;

  TH1F* mHistDeltatPi[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH1F* mHistDeltatKa[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH1F* mHistDeltatPr[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH2F* mHistDeltatPiPt[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH2F* mHistDeltatKaPt[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH2F* mHistDeltatPrPt[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH1F* mHistMass[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH2F* mHistBetavsP[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH2F* mHistMassvsP[trackType::SIZE][evTimeType::SIZEt0] = {};
  TH2F* mHistDeltatPiEvTimeRes[trackType::SIZE] = {};
  TH2F* mHistDeltatPiEvTimeMult[trackType::SIZE] = {};
  TH2F* mHistEvTimeResEvTimeMult = 0x0;
  TH1F* mHistEvTimeTOF = 0x0;
  TH2F* mHistEvTimeTOFVsFT0AC = 0x0;
  TH2F* mHistEvTimeTOFVsFT0A = 0x0;
  TH2F* mHistEvTimeTOFVsFT0C = 0x0;
  TH1F* mHistDeltaEvTimeTOFVsFT0AC = 0x0;
  TH1F* mHistDeltaEvTimeTOFVsFT0A = 0x0;
  TH1F* mHistDeltaEvTimeTOFVsFT0C = 0x0;
  TH2F* mHistEvTimeTOFVsFT0ACSameBC = 0x0;
  TH2F* mHistEvTimeTOFVsFT0ASameBC = 0x0;
  TH2F* mHistEvTimeTOFVsFT0CSameBC = 0x0;
  TH1F* mHistDeltaEvTimeTOFVsFT0ACSameBC = 0x0;
  TH1F* mHistDeltaEvTimeTOFVsFT0ASameBC = 0x0;
  TH1F* mHistDeltaEvTimeTOFVsFT0CSameBC = 0x0;
  TH1I* mHistDeltaBCTOFFT0 = 0x0;
};

} // namespace o2::quality_control_modules::pid

#endif // QC_MODULE_PID_TASKFT0TOF_H
