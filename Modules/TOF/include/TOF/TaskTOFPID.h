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
/// \file   TaskTOFPID.h
/// \author Francesca Ercolessi
/// \brief  Header task to monitor TOF PID performance
/// \since  13/01/2022
///

#ifndef QC_MODULE_TOF_TOFTASKTOFPID_H
#define QC_MODULE_TOF_TOFTASKTOFPID_H

#include "QualityControl/TaskInterface.h"

#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "ReconstructionDataFormats/MatchInfoTOFReco.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"

class TH1F;
class TH2F;

namespace o2::quality_control_modules::tof
{

using namespace o2::quality_control::core;
using GID = o2::dataformats::GlobalTrackID;
using trkType = o2::dataformats::MatchInfoTOFReco::TrackType;

struct MyTrack {
  o2::dataformats::TrackTPCITS trk;
  o2::dataformats::MatchInfoTOF match;
  MyTrack(const o2::dataformats::MatchInfoTOF& m, const o2::dataformats::TrackTPCITS& t) : match(m), trk(t) {}
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
  const o2::dataformats::TrackTPCITS& getTrack() { return trk; }
};

class TaskTOFPID final : public TaskInterface
{
 public:
  /// \brief Constructor
  TaskTOFPID() = default;
  /// Destructor
  ~TaskTOFPID() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  void processEvent(const std::vector<MyTrack>& tracks);
  // track selection
  bool selectTrack(o2::tpc::TrackTPC const& track);
  void setPtCut(float v) { mPtCut = v; }
  void setEtaCut(float v) { mEtaCut = v; }
  void setMinNTPCClustersCut(float v) { mNTPCClustersCut = v; }
  void setMinDCAtoBeamPipeCut(std::array<float, 2> v)
  {
    setMinDCAtoBeamPipeDistanceCut(v[0]);
    setMinDCAtoBeamPipeYCut(v[1]);
  }
  void setMinDCAtoBeamPipeDistanceCut(float v) { mDCACut = v; }
  void setMinDCAtoBeamPipeYCut(float v) { mDCACutY = v; }

 private:
  std::shared_ptr<o2::globaltracking::DataRequest> mDataRequest;
  o2::globaltracking::RecoContainer mRecoCont;
  GID::mask_t mSrc = GID::getSourcesMask("ITS-TPC");
  GID::mask_t mAllowedSources = GID::getSourcesMask("TPC,ITS-TPC,TPC-TOF,ITS-TPC-TOF");
  // TPC-TOF
  gsl::span<const o2::tpc::TrackTPC> mTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mTPCTOFMatches;
  // ITS-TPC-TOF
  gsl::span<const o2::dataformats::TrackTPCITS> mITSTPCTracks;
  gsl::span<const o2::dataformats::MatchInfoTOF> mITSTPCTOFMatches;
  std::vector<MyTrack> mMyTracks;

  // for track selection
  float mPtCut = 0.1f;
  float mEtaCut = 0.8f;
  int32_t mNTPCClustersCut = 40;
  float mDCACut = 100.f;
  float mDCACutY = 10.f;
  std::string mGRPFileName = "o2sim_grp.root";
  std::string mGeomFileName = "o2sim_geometry.root";
  float mBz = 0; ///< nominal Bz
  int mTF = -1;  // to count the number of processed TFs
  const float cinv = 33.35641;

  TH1F* mHistDeltatPi;
  TH1F* mHistDeltatKa;
  TH1F* mHistDeltatPr;
  TH2F* mHistDeltatPiPt;
  TH2F* mHistDeltatKaPt;
  TH2F* mHistDeltatPrPt;
  TH1F* mHistMass;
  TH2F* mHistBetavsP;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTaskTOFPID_H
