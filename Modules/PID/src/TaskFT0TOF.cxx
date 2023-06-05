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
/// \file   TaskFT0TOF.cxx
/// \author Francesca Ercolessi
/// \brief  Task to monitor TOF PID performance
/// \since  13/01/2022
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TLorentzVector.h>

#include "QualityControl/QcInfoLogger.h"
#include "PID/TaskFT0TOF.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpec.h>
#include "GlobalTrackingWorkflowHelpers/InputHelper.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "DataFormatsGlobalTracking/RecoContainerCreateTracksVariadic.h"
#include "ReconstructionDataFormats/TrackParametrization.h"
#include "DetectorsBase/Propagator.h"
#include "TOFBase/EventTimeMaker.h"
#include "GlobalTrackingWorkflow/TOFEventTimeChecker.h"
#include "DetectorsRaw/HBFUtils.h"

// from TOF
#include "TOFBase/Geo.h"
#include "TOFBase/Utils.h"
#include "DataFormatsTOF/Cluster.h"
#include "TOFBase/EventTimeMaker.h"

using GTrackID = o2::dataformats::GlobalTrackID;
using namespace o2::tof;

namespace o2::quality_control_modules::pid
{

float MyTrack::t0maxp = 1.5; // default p threshold for tracks for event time computation

bool MyFilter(const MyTrack& tr)
{
  return (tr.getP() < MyTrack::getT0MaxP());
}

TaskFT0TOF::~TaskFT0TOF()
{
  // delete
  for (int i = 0; i < trackType::SIZE; ++i) {
    for (int j = 0; j < evTimeType::SIZEt0; ++j) {
      delete mHistDeltatPi[i][j];
      delete mHistDeltatKa[i][j];
      delete mHistDeltatPr[i][j];
      delete mHistDeltatPiPt[i][j];
      delete mHistDeltatKaPt[i][j];
      delete mHistDeltatPrPt[i][j];
      delete mHistMass[i][j];
      delete mHistMassvsP[i][j];
      delete mHistBetavsP[i][j];
    }
    delete mHistDeltatPiEvTimeRes[i];
    delete mHistDeltatPiEvTimeMult[i];
  }
  delete mHistEvTimeResEvTimeMult;
  delete mHistEvTimeTOF;
  delete mHistEvTimeTOFVsFT0AC;
  delete mHistEvTimeTOFVsFT0A;
  delete mHistEvTimeTOFVsFT0C;
  delete mHistDeltaEvTimeTOFVsFT0AC;
  delete mHistDeltaEvTimeTOFVsFT0A;
  delete mHistDeltaEvTimeTOFVsFT0C;
  delete mHistEvTimeTOFVsFT0ACSameBC;
  delete mHistEvTimeTOFVsFT0ASameBC;
  delete mHistEvTimeTOFVsFT0CSameBC;
  delete mHistDeltaEvTimeTOFVsFT0ACSameBC;
  delete mHistDeltaEvTimeTOFVsFT0ASameBC;
  delete mHistDeltaEvTimeTOFVsFT0CSameBC;
  delete mHistDeltaBCTOFFT0;
  delete mHistMismatchVsEta;
  delete mProfLoverCvsEta;
}

void TaskFT0TOF::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << " Initializing... " << ENDM;
  // track selection
  if (auto param = mCustomParameters.find("minPtCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minPtCut (for track selection): " << param->second << ENDM;
    setMinPtCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("etaCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - etaCut (for track selection): " << param->second << ENDM;
    setEtaCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minNTPCClustersCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minNTPCClustersCut (for track selection): " << param->second << ENDM;
    setMinNTPCClustersCut(atoi(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minDCACut (for track selection): " << param->second << ENDM;
    setMinDCAtoBeamPipeCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACutY"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minDCACutY (for track selection): " << param->second << ENDM;
    setMinDCAtoBeamPipeYCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("useFT0"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - useFT0: " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mUseFT0 = true;
    }
  }
  if (auto param = mCustomParameters.find("evTimeTracksMaxMomentum"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - EvTimeTracksMaxMomentum (for ev time computation): " << param->second << ENDM;
    mEvTimeTracksMaxMomentum = atof(param->second.c_str());
    MyTrack::setT0MaxP(mEvTimeTracksMaxMomentum);
  }

  // for track type selection
  if (auto param = mCustomParameters.find("GID"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - GID (= sources by user): " << param->second << ENDM;
    ILOG(Info, Devel) << "Allowed Sources = " << mAllowedSources << ENDM;
    mSrc = mAllowedSources & GID::getSourcesMask(param->second);
    ILOG(Info, Devel) << "Final requested sources = " << mSrc << ENDM;
  }

  // TPC tracks
  if ((mSrc[GID::Source::TPCTOF] == 1 && mSrc[GID::Source::TPC] == 0) || (mSrc[GID::Source::TPCTOF] == 0 && mSrc[GID::Source::TPC] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: TPCTOF = " << mSrc[GID::Source::TPCTOF] << ", TPC = " << mSrc[GID::Source::TPC] << ENDM;
  }

  // ITS-TPC tracks
  if ((mSrc[GID::Source::ITSTPCTOF] == 1 && mSrc[GID::Source::ITSTPC] == 0) || (mSrc[GID::Source::ITSTPCTOF] == 0 && mSrc[GID::Source::ITSTPC] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: ITSTPCTOF = " << mSrc[GID::Source::ITSTPCTOF] << ", ITSTPC = " << mSrc[GID::Source::ITSTPC] << ENDM;
  }

  // TPC-TRD tracks
  if ((mSrc[GID::Source::TPCTRDTOF] == 1 && mSrc[GID::Source::TPCTRD] == 0) || (mSrc[GID::Source::TPCTRDTOF] == 0 && mSrc[GID::Source::TPCTRD] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: TPCTRDTOF = " << mSrc[GID::Source::TPCTRDTOF] << ", TPCTRD = " << mSrc[GID::Source::TPCTRD] << ENDM;
  }

  // ITS-TPC-TRD tracks
  if ((mSrc[GID::Source::ITSTPCTRDTOF] == 1 && mSrc[GID::Source::ITSTPCTRD] == 0) || (mSrc[GID::Source::ITSTPCTRDTOF] == 0 && mSrc[GID::Source::ITSTPCTRD] == 1)) {
    ILOG(Fatal, Support) << "Check the requested sources: ITSTPCTRDTOF = " << mSrc[GID::Source::ITSTPCTRDTOF] << ", ITSTPCTRD = " << mSrc[GID::Source::ITSTPCTRD] << ENDM;
  }

  // initialize histgrams
  std::array<std::string, 4> title{ "TPC", "ITSTPC", "ITSTPCTRD", "TPCTRD" };
  std::array<std::string, 4> evtimetitle{ "TOF", "FT0AC", "FT0A", "FT0C" };
  for (int i = 0; i < trackType::SIZE; ++i) {
    for (int j = 0; j < evTimeType::SIZEt0; ++j) {
      mHistDeltatPi[i][j] = new TH1F(Form("DeltatPi_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;t_{TOF} - t_{exp}^{#pi} (ps)", title[i].c_str(), evtimetitle[j].c_str()), 500, -5000, 5000);
      mHistDeltatKa[i][j] = new TH1F(Form("DeltatKa_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;t_{TOF} - t_{exp}^{K} (ps)", title[i].c_str(), evtimetitle[j].c_str()), 500, -5000, 5000);
      mHistDeltatPr[i][j] = new TH1F(Form("DeltatPr_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;t_{TOF} - t_{exp}^{p} (ps)", title[i].c_str(), evtimetitle[j].c_str()), 500, -5000, 5000);
      mHistDeltatPiPt[i][j] = new TH2F(Form("DeltatPi_Pt_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;#it{p}_{T} (GeV/#it{c});t_{TOF} - t_{exp}^{#pi} (ps)", title[i].c_str(), evtimetitle[j].c_str()), 100, 0., 10., 500, -5000, 5000);
      mHistDeltatKaPt[i][j] = new TH2F(Form("DeltatKa_Pt_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;#it{p}_{T} (GeV/#it{c});t_{TOF} - t_{exp}^{#pi} (ps)", title[i].c_str(), evtimetitle[j].c_str()), 100, 0., 10., 500, -5000, 5000);
      mHistDeltatPrPt[i][j] = new TH2F(Form("DeltatPr_Pt_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;#it{p}_{T} (GeV/#it{c});t_{TOF} - t_{exp}^{#pi} (ps)", title[i].c_str(), evtimetitle[j].c_str()), 100, 0., 10., 500, -5000, 5000);
      mHistMass[i][j] = new TH1F(Form("HadronMasses_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;M (GeV/#it{c}^{2})", title[i].c_str(), evtimetitle[j].c_str()), 1000, 0, 3.);
      mHistMassvsP[i][j] = new TH2F(Form("HadronMassesvsP_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;#it{p} (GeV/#it{c});M (GeV/#it{c}^{2})", title[i].c_str(), evtimetitle[j].c_str()), 100, 0., 5, 500, 0, 3.);
      mHistBetavsP[i][j] = new TH2F(Form("BetavsP_%s_t0%s", title[i].c_str(), evtimetitle[j].c_str()), Form("tracks: %s , evTime: %s;#it{p} (GeV/#it{c});TOF #beta", title[i].c_str(), evtimetitle[j].c_str()), 100, 0., 5, 500, 0., 1.5);
    }
    mHistDeltatPiEvTimeRes[i] = new TH2F(Form("DeltatPiEvtimeRes_%s", title[i].c_str()), Form("tracks %s, 1.5 < p < 1.6 GeV/#it{c};TOF event time resolution (ps);t_{TOF} - t_{exp}^{#pi} (ps)", title[i].c_str()), 100, 0., 200, 500, -5000, 5000);
    mHistDeltatPiEvTimeMult[i] = new TH2F(Form("DeltatPiEvTimeMult_%s", title[i].c_str()), Form("tracks %s, 1.5 < p < 1.6 GeV/#it{c};TOF multiplicity; t_{TOF} - t_{exp}^{#pi} (ps)", title[i].c_str()), 200, 0., 200, 500, -5000, 5000);
  }
  mHistEvTimeResEvTimeMult = new TH2F("EvTimeResEvTimeMult", "1.5 < p < 1.6 GeV/#it{c};TOF multiplicity;TOF event time resolution (ps)", 100, 0., 100, 200, 0, 200);
  mHistEvTimeTOF = new TH1F("EvTimeTOF", "t_{0}^{TOF};t_{0}^{TOF} (ps);Counts", 1000, -5000., +5000);
  mHistEvTimeTOFVsFT0AC = new TH2F("EvTimeTOFVsFT0AC", "t_{0}^{FT0AC} vs t_{0}^{TOF} w.r.t. BC;t_{0}^{TOF} w.r.t. BC (ps);t_{0}^{FT0AC} w.r.t. BC (ps)", 200, -2000., +2000, 200, -2000., +2000);
  mHistEvTimeTOFVsFT0A = new TH2F("EvTimeTOFVsFT0A", "t_{0}^{FT0A} vs t_{0}^{TOF} w.r.t. BC;t_{0}^{TOF} w.r.t. BC (ps);t_{0}^{FT0A} w.r.t. BC (ps)", 200, -2000., +2000, 200, -2000., +2000);
  mHistEvTimeTOFVsFT0C = new TH2F("EvTimeTOFVsFT0C", "t_{0}^{FT0C} vs t_{0}^{TOF} w.r.t. BC;t_{0}^{TOF} w.r.t. BC (ps);t_{0}^{FT0C} w.r.t. BC (ps)", 200, -2000., +2000, 200, -2000., +2000);
  mHistDeltaEvTimeTOFVsFT0AC = new TH1F("DeltaEvTimeTOFVsFT0AC", ";t_{0}^{TOF} - t_{0}^{FT0AC} (ps)", 200, -2000., +2000);
  mHistDeltaEvTimeTOFVsFT0A = new TH1F("DeltaEvTimeTOFVsFT0A", ";t_{0}^{TOF} - t_{0}^{FT0A} (ps)", 200, -2000., +2000);
  mHistDeltaEvTimeTOFVsFT0C = new TH1F("DeltaEvTimeTOFVsFT0C", ";t_{0}^{TOF} - t_{0}^{FT0C} (ps)", 200, -2000., +2000);
  mHistEvTimeTOFVsFT0ACSameBC = new TH2F("EvTimeTOFVsFT0ACSameBC", "t_{0}^{FT0AC} vs t_{0}^{TOF} w.r.t. BC;t_{0}^{TOF} w.r.t. BC (ps);t_{0}^{FT0AC} w.r.t. BC (ps)", 200, -2000., +2000, 200, -2000., +2000);
  mHistEvTimeTOFVsFT0ASameBC = new TH2F("EvTimeTOFVsFT0ASameBC", "t_{0}^{FT0A} vs t_{0}^{TOF} w.r.t. BC;t_{0}^{TOF} w.r.t. BC (ps);t_{0}^{FT0A} w.r.t. BC (ps)", 200, -2000., +2000, 200, -2000., +2000);
  mHistEvTimeTOFVsFT0CSameBC = new TH2F("EvTimeTOFVsFT0CSameBC", "t_{0}^{FT0C} vs t_{0}^{TOF} w.r.t. BC;t_{0}^{TOF} w.r.t. BC (ps);t_{0}^{FT0C} w.r.t. BC (ps)", 200, -2000., +2000, 200, -2000., +2000);
  mHistDeltaEvTimeTOFVsFT0ACSameBC = new TH1F("DeltaEvTimeTOFVsFT0ACSameBC", ";t_{0}^{TOF} - t_{0}^{FT0AC} (ps)", 200, -2000., +2000);
  mHistDeltaEvTimeTOFVsFT0ASameBC = new TH1F("DeltaEvTimeTOFVsFT0ASameBC", ";t_{0}^{TOF} - t_{0}^{FT0A} (ps)", 200, -2000., +2000);
  mHistDeltaEvTimeTOFVsFT0CSameBC = new TH1F("DeltaEvTimeTOFVsFT0CSameBC", ";t_{0}^{TOF} - t_{0}^{FT0C} (ps)", 200, -2000., +2000);
  mHistDeltaBCTOFFT0 = new TH1I("DeltaBCTOFFT0", "#Delta BC (TOF-FT0 evt time);#Delta BC", 16, -8, +8);
  mHistMismatchVsEta = new TH2F("mHistMismatchVsEta", ";#eta;t_{TOF}-t_{0}^{FT0AC}-L_{ch}/c", 21, -1., +1., 6500, -30000, +100000);
  mProfLoverCvsEta = new TProfile("LoverCvsEta", ";#eta;L_{ch}/c", 21, -1., +1.);

  // publish histgrams
  getObjectsManager()->startPublishing(mHistEvTimeResEvTimeMult);
  getObjectsManager()->startPublishing(mHistEvTimeTOF);
  getObjectsManager()->startPublishing(mHistDeltaBCTOFFT0);
  getObjectsManager()->startPublishing(mHistEvTimeTOFVsFT0AC);
  getObjectsManager()->startPublishing(mHistEvTimeTOFVsFT0A);
  getObjectsManager()->startPublishing(mHistEvTimeTOFVsFT0C);
  getObjectsManager()->startPublishing(mHistDeltaEvTimeTOFVsFT0AC);
  getObjectsManager()->startPublishing(mHistDeltaEvTimeTOFVsFT0A);
  getObjectsManager()->startPublishing(mHistDeltaEvTimeTOFVsFT0C);
  getObjectsManager()->startPublishing(mHistEvTimeTOFVsFT0ACSameBC);
  getObjectsManager()->startPublishing(mHistEvTimeTOFVsFT0ASameBC);
  getObjectsManager()->startPublishing(mHistEvTimeTOFVsFT0CSameBC);
  getObjectsManager()->startPublishing(mHistDeltaEvTimeTOFVsFT0ACSameBC);
  getObjectsManager()->startPublishing(mHistDeltaEvTimeTOFVsFT0ASameBC);
  getObjectsManager()->startPublishing(mHistDeltaEvTimeTOFVsFT0CSameBC);
  getObjectsManager()->startPublishing(mHistMismatchVsEta);
  getObjectsManager()->startPublishing(mProfLoverCvsEta);

  // Use FT0?
  int evTimeMax = 1; // if not use only TOF (evTimeType::TOF == 0)
  if (mUseFT0) {
    evTimeMax = evTimeType::SIZEt0;
  }

  // Is track TPC-TOF?
  if (mSrc[GID::Source::ITSTPCTOF] == 1) {
    for (int j = 0; j < evTimeMax; ++j) {
      getObjectsManager()->startPublishing(mHistDeltatPi[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatKa[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatPr[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatPiPt[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatKaPt[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatPrPt[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistMass[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistBetavsP[trackType::TPC][j]);
      getObjectsManager()->startPublishing(mHistMassvsP[trackType::TPC][j]);
    }
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeRes[trackType::TPC]);
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeMult[trackType::TPC]);
  }

  // Is track TPC-TRD-TOF?
  if (mSrc[GID::Source::TPCTRDTOF] == 1) {
    for (int j = 0; j < evTimeMax; ++j) {
      getObjectsManager()->startPublishing(mHistDeltatPi[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatKa[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatPr[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatPiPt[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatKaPt[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatPrPt[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistMass[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistBetavsP[trackType::TPCTRD][j]);
      getObjectsManager()->startPublishing(mHistMassvsP[trackType::TPCTRD][j]);
    }
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeRes[trackType::TPCTRD]);
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeMult[trackType::TPCTRD]);
  }
  // Is track ITS-TPC-TOF
  if (mSrc[GID::Source::ITSTPCTOF] == 1) {
    for (int j = 0; j < evTimeMax; ++j) {
      getObjectsManager()->startPublishing(mHistDeltatPi[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatKa[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatPr[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatPiPt[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatKaPt[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistDeltatPrPt[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistMass[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistBetavsP[trackType::ITSTPC][j]);
      getObjectsManager()->startPublishing(mHistMassvsP[trackType::ITSTPC][j]);
    }
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeRes[trackType::ITSTPC]);
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeMult[trackType::ITSTPC]);
  }
  // Is track ITS-TPC-TRD-TOF
  if (mSrc[GID::Source::ITSTPCTRDTOF] == 1) {
    for (int j = 0; j < evTimeMax; ++j) {
      getObjectsManager()->startPublishing(mHistDeltatPi[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatKa[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatPr[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatPiPt[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatKaPt[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistDeltatPrPt[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistMass[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistBetavsP[trackType::ITSTPCTRD][j]);
      getObjectsManager()->startPublishing(mHistMassvsP[trackType::ITSTPCTRD][j]);
    }
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeRes[trackType::ITSTPCTRD]);
    getObjectsManager()->startPublishing(mHistDeltatPiEvTimeMult[trackType::ITSTPCTRD]);
  }
  ILOG(Info, Support) << " Initialized!!!! " << ENDM;

  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrc, 0 /*mUseMC*/);
}

void TaskFT0TOF::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void TaskFT0TOF::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TaskFT0TOF::monitorData(o2::framework::ProcessingContext& ctx)
{
  ++mTF;
  ILOG(Info, Support) << " Processing TF: " << mTF << ENDM;

  // Getting the B field
  mBz = o2::base::Propagator::Instance()->getNominalBz();

  mRecoCont.collectData(ctx, *mDataRequest.get());

  mMyTracks.clear();

  // Get FT0 RecPointss
  // const gsl::span<o2::ft0::RecPoints>* ft0rec = nullptr;
  const std::vector<o2::ft0::RecPoints>* ft0rec = nullptr;
  if (mUseFT0) {
    auto& obj = ctx.inputs().get<const std::vector<o2::ft0::RecPoints>>("recpoints");
    // const auto& obj = mRecoCont.getFT0RecPoints();
    ft0rec = &obj;
  }

  if (mUseFT0) {
    ILOG(Info, Support) << "FT0 rec points loaded, size = " << ft0rec->size() << ENDM;
  } else {
    ILOG(Info, Support) << "FT0 rec points NOT available" << ENDM;
  }

  // TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTOF)) { // if track is TPC

    mTPCTracks = mRecoCont.getTPCTracks();
    mTPCTOFMatches = mRecoCont.getTPCTOFMatches();

    if (mRecoCont.getTPCTOFTracks().size() != mTPCTOFMatches.size()) {
      ILOG(Fatal, Support) << "Number of TPCTOF tracks (" << mRecoCont.getTPCTOFTracks().size() << ") differs from number of TPCTOF matches (" << mTPCTOFMatches.size() << ")" << ENDM;
    }

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mTPCTracks[gTrackId.getIndex()];

      if (!selectTrack(trk)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trk, trackType::TPC));
      auto& mytrk = mMyTracks[mMyTracks.size() - 1];
      mytrk.setP(trk.getP());
      mytrk.setPt(trk.getPt());
    } // END loop on TOF matches
  }   // END if track is TPC-TOF

  // ITS-TPC-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTOF)) { // if track is ITS+TPC

    mITSTPCTracks = mRecoCont.getTPCITSTracks();
    mITSTPCTOFMatches = mRecoCont.getITSTPCTOFMatches();
    mTPCTracks = mRecoCont.getTPCTracks();

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTracks[gTrackId.getIndex()];
      const auto& trkTPC = mTPCTracks[trk.getRefTPC()];

      if (!selectTrack(trkTPC)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trkTPC, trackType::ITSTPC));
      auto& mytrk = mMyTracks[mMyTracks.size() - 1];
      mytrk.setP(trk.getP());
      mytrk.setPt(trk.getPt());
    } // END loop on TOF matches
  }   // END if track is ITS-TPC-TOF

  // TPC-TRD-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::TPCTRDTOF)) { // this is enough to know that also TPCTRD was loades, see "initialize"

    mTPCTRDTracks = mRecoCont.getTPCTRDTracks<o2::trd::TrackTRD>();
    mTPCTRDTOFMatches = mRecoCont.getTPCTRDTOFMatches();

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mTPCTRDTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mTPCTRDTracks[gTrackId.getIndex()];
      const auto& trkTPC = mTPCTracks[trk.getRefGlobalTrackId()];

      if (!selectTrack(trkTPC)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trkTPC, trackType::TPCTRD));
      auto& mytrk = mMyTracks[mMyTracks.size() - 1];
      mytrk.setP(trk.getP());
      mytrk.setPt(trk.getPt());
    } // END loop on TOF matches
  }   // END if track is TPC-TRD-TOF

  // ITS-TPC-TRD-TOF
  if (mRecoCont.isTrackSourceLoaded(GID::ITSTPCTRDTOF)) { // this is enough to know that also ITSTPC was loades, see "initialize"

    mITSTPCTRDTracks = mRecoCont.getITSTPCTRDTracks<o2::trd::TrackTRD>();
    mITSTPCTRDTOFMatches = mRecoCont.getITSTPCTRDTOFMatches();

    // loop over TOF MatchInfo
    for (const auto& matchTOF : mITSTPCTRDTOFMatches) {

      GTrackID gTrackId = matchTOF.getTrackRef();
      const auto& trk = mITSTPCTRDTracks[gTrackId.getIndex()];
      const auto& trkITSTPC = mITSTPCTracks[trk.getRefGlobalTrackId()];
      const auto& trkTPC = mTPCTracks[trkITSTPC.getRefTPC()];

      if (!selectTrack(trkTPC)) { // Discard tracks according to configurable cuts
        continue;
      }

      mMyTracks.push_back(MyTrack(matchTOF, trkTPC, trackType::ITSTPCTRD));
      auto& mytrk = mMyTracks[mMyTracks.size() - 1];
      mytrk.setP(trk.getP());
      mytrk.setPt(trk.getPt());
    } // END loop on TOF matches
  }   // END if track is ITS-TPC-TRD-TOF

  std::vector<MyTrack> tracks;
  std::vector<o2::ft0::RecPoints> ft0Sorted;

  if (mUseFT0) {
    ft0Sorted = *ft0rec;
  }
  std::vector<o2::ft0::RecPoints> ft0Cand;

  // sorting matching in time
  std::sort(mMyTracks.begin(), mMyTracks.end(),
            [](MyTrack a, MyTrack b) { return a.tofSignalDouble() < b.tofSignalDouble(); });

  std::sort(ft0Sorted.begin(), ft0Sorted.end(),
            [](o2::ft0::RecPoints a, o2::ft0::RecPoints b) { return a.getInteractionRecord().bc2ns() < b.getInteractionRecord().bc2ns(); });

  int ift0 = 0;

  uint32_t ft0firstOrbit = ctx.services().get<o2::framework::TimingInfo>().firstTForbit;

  //  ILOG(Info, Support) << "ft0firstOrbit = " << ft0firstOrbit << " - BC = " <<  mRecoCont.startIR.bc <<ENDM;

  for (int i = 0; i < mMyTracks.size(); i++) { // loop looking for interaction candidates
    tracks.clear();
    ft0Cand.clear();

    int ntrk = 1;
    double time = mMyTracks[i].tofSignalDouble();

    tracks.emplace_back(mMyTracks[i]);
    for (; i < mMyTracks.size(); i++) {
      double timeCurrent = mMyTracks[i].tofSignalDouble();
      if (timeCurrent - time > 100E3) {
        i--;
        break;
      }
      tracks.emplace_back(mMyTracks[i]);
      ntrk++;
    }
    if (ntrk > 0) { // good candidate with time

      // select FT0 candidates within 8 BCs
      if (mUseFT0) {
        double firstTime = tracks[0].tofSignalDouble() - 8 * o2::tof::Geo::BC_TIME_INPS;
        double lastTime = tracks[ntrk - 1].tofSignalDouble() + 8 * o2::tof::Geo::BC_TIME_INPS;
        for (int j = ift0; j < ft0Sorted.size(); j++) {
          auto& obj = ft0Sorted[j];
          if (obj.getInteractionRecord().orbit < ft0firstOrbit) {
            continue; // skip FT0 objects from previous orbits
          }
          uint32_t orbit = obj.getInteractionRecord().orbit - ft0firstOrbit;
          double BCtimeFT0 = ((orbit)*o2::constants::lhc::LHCMaxBunches + obj.getInteractionRecord().bc) * o2::tof::Geo::BC_TIME_INPS;

          if (BCtimeFT0 < firstTime) {
            ift0 = j + 1;
            continue;
          }
          if (BCtimeFT0 > lastTime) {
            break;
          }
          //          ILOG(Info, Support) << " TOF first=" << firstTime << " < " << BCtimeFT0 << " < TOF last=" << lastTime << ENDM;
          //          ILOG(Info, Support) << " orbit=" << obj.getInteractionRecord().orbit << " " << obj.getInteractionRecord().orbit - ft0firstOrbit << " BC=" << obj.getInteractionRecord().bc << ENDM;

          std::array<short, 4> collTimes = { obj.getCollisionTime(0), obj.getCollisionTime(1), obj.getCollisionTime(2), obj.getCollisionTime(3) };

          int pos = ft0Cand.size();
          ft0Cand.emplace_back(collTimes, 0, 0, o2::InteractionRecord{ obj.getInteractionRecord().bc, orbit }, obj.getTrigger());
        }
      }

      processEvent(tracks, ft0Cand);
    }
  } // end loop on tracks

  const auto clusters = mRecoCont.getTOFClusters(); // get the TOF clusters
  std::vector<int> clsIndex;                        // use vector of clusters indices
  clsIndex.resize(clusters.size());
  for (int i = 0; i < clsIndex.size(); i++) {
    clsIndex[i] = i;
  }
  // sort clusters indices in time
  std::sort(clsIndex.begin(), clsIndex.end(), [&clusters](int a, int b) { return clusters[a].getTime() < clusters[b].getTime(); });

  int icls = 0;
  static TLorentzVector v;

  for (auto& ft0 : ft0Sorted) {
    if (!ft0.isValidTime(0)) {
      continue; // skip invalid FT0AC times
    }
    if (ft0firstOrbit > ft0.getInteractionRecord().orbit) {
      continue; // skip FT0 objects from previous orbits
    }

    double ft0time = ((ft0.getInteractionRecord().orbit - ft0firstOrbit) * o2::constants::lhc::LHCMaxBunches + ft0.getInteractionRecord().bc) * o2::tof::Geo::BC_TIME_INPS + ft0.getCollisionTime(0);
    double timemax = ft0time + 100E3;
    double timemin = ft0time - 30E3;

    for (int j = icls; j < clusters.size(); j++) {
      auto& obj = clusters[clsIndex[j]];

      if (obj.getTime() < timemin) {
        icls = j + 1;
        continue;
      }
      if (obj.getTime() > timemax) {
        break;
      }

      int channel = obj.getMainContributingChannel(); // channel index
      int det[5];
      o2::tof::Geo::getVolumeIndices(channel, det); // detector index
      float pos[3];
      o2::tof::Geo::getPos(det, pos); // detector position
      v.SetXYZT(pos[0], pos[1], pos[2], 0);
      float LoverC = v.P() * 33.3564095; // L/c
      float eta = v.Eta();

      mHistMismatchVsEta->Fill(eta, obj.getTime() - ft0time - LoverC);
      mProfLoverCvsEta->Fill(eta, LoverC);
    }
  }

  ILOG(Info, Support) << " Processed! " << ENDM;
  return;
}

void TaskFT0TOF::endOfCycle()
{

  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TaskFT0TOF::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TaskFT0TOF::reset()
{
  // clean all the monitor objects here
  for (int i = 0; i < trackType::SIZE; ++i) {
    for (int j = 0; j < evTimeType::SIZEt0; ++j) {
      mHistDeltatPi[i][j]->Reset();
      mHistDeltatKa[i][j]->Reset();
      mHistDeltatPr[i][j]->Reset();
      mHistDeltatPiPt[i][j]->Reset();
      mHistDeltatKaPt[i][j]->Reset();
      mHistDeltatPrPt[i][j]->Reset();
      mHistMass[i][j]->Reset();
      mHistBetavsP[i][j]->Reset();
      mHistMassvsP[i][j]->Reset();
    }
    mHistDeltatPiEvTimeRes[i]->Reset();
    mHistDeltatPiEvTimeMult[i]->Reset();
  }
  mHistEvTimeResEvTimeMult->Reset();
  mHistEvTimeTOF->Reset();
  mHistEvTimeTOFVsFT0AC->Reset();
  mHistEvTimeTOFVsFT0A->Reset();
  mHistEvTimeTOFVsFT0C->Reset();
  mHistDeltaEvTimeTOFVsFT0AC->Reset();
  mHistDeltaEvTimeTOFVsFT0A->Reset();
  mHistDeltaEvTimeTOFVsFT0C->Reset();
  mHistEvTimeTOFVsFT0ACSameBC->Reset();
  mHistEvTimeTOFVsFT0ASameBC->Reset();
  mHistEvTimeTOFVsFT0CSameBC->Reset();
  mHistDeltaEvTimeTOFVsFT0ACSameBC->Reset();
  mHistDeltaEvTimeTOFVsFT0ASameBC->Reset();
  mHistDeltaEvTimeTOFVsFT0CSameBC->Reset();
}

void TaskFT0TOF::processEvent(const std::vector<MyTrack>& tracks, const std::vector<o2::ft0::RecPoints>& ft0Cand)
{
  auto evtime = o2::tof::evTimeMaker<std::vector<MyTrack>, MyTrack, MyFilter>(tracks);
  // if event time is 0
  if (evtime.mEventTimeMultiplicity <= 2) {
    int nBC = (tracks[0].tofSignal()) * o2::tof::Geo::BC_TIME_INPS_INV;
    evtime.mEventTime = nBC * o2::tof::Geo::BC_TIME_INPS;
    evtime.mEventTimeError = 200;
  }
  bool isTOFst = evtime.mEventTimeError < 150;
  int nBC = (evtime.mEventTime + 5000.) * o2::tof::Geo::BC_TIME_INPS_INV;  // 5 ns offset to get the correct number of BC (int)
  float mEvTime_BC = evtime.mEventTime - nBC * o2::tof::Geo::BC_TIME_INPS; // Event time in ps wrt BC
                                                                           //
  if (TMath::Abs(mEvTime_BC) > 800) {
    mEvTime_BC = 0;
    isTOFst = false;
    evtime.mEventTime = nBC * o2::tof::Geo::BC_TIME_INPS;
    evtime.mEventTimeMultiplicity = 0;
  }

  float FT0evTimes[3] = {};
  for (auto& obj : ft0Cand) { // fill histo for FT0
    bool isAC = obj.isValidTime(0);
    bool isA = obj.isValidTime(1);
    bool isC = obj.isValidTime(2);
    // t0 times w.r.t. BC
    float times[4] = { mEvTime_BC, isAC ? obj.getCollisionTime(0) : 0.f, isC ? obj.getCollisionTime(1) : 0.f, isC ? obj.getCollisionTime(2) : 0.f }; // TOF, FT0-AC, FT0-A, FT0-C
    if (isTOFst) {
      // no condition for same BC
      mHistEvTimeTOFVsFT0AC->Fill(times[0], times[1]);
      mHistEvTimeTOFVsFT0A->Fill(times[0], times[2]);
      mHistEvTimeTOFVsFT0C->Fill(times[0], times[3]);
      mHistDeltaEvTimeTOFVsFT0AC->Fill(times[0] - times[1]);
      mHistDeltaEvTimeTOFVsFT0A->Fill(times[0] - times[2]);
      mHistDeltaEvTimeTOFVsFT0C->Fill(times[0] - times[3]);

      // if same BC
      if (nBC % o2::constants::lhc::LHCMaxBunches == obj.getInteractionRecord().bc) {
        // no need for condition on orbit we select FT0 candidates within 8 BCs
        FT0evTimes[0] = obj.getInteractionRecord().bc2ns() * 1E3 + times[1];
        FT0evTimes[1] = obj.getInteractionRecord().bc2ns() * 1E3 + times[2];
        FT0evTimes[2] = obj.getInteractionRecord().bc2ns() * 1E3 + times[3];
        mHistEvTimeTOFVsFT0ACSameBC->Fill(times[0], times[1]);
        mHistEvTimeTOFVsFT0ASameBC->Fill(times[0], times[2]);
        mHistEvTimeTOFVsFT0CSameBC->Fill(times[0], times[3]);
        mHistDeltaEvTimeTOFVsFT0ACSameBC->Fill(times[0] - times[1]);
        mHistDeltaEvTimeTOFVsFT0ASameBC->Fill(times[0] - times[2]);
        mHistDeltaEvTimeTOFVsFT0CSameBC->Fill(times[0] - times[3]);
      }

      mHistDeltaBCTOFFT0->Fill(nBC % o2::constants::lhc::LHCMaxBunches - obj.getInteractionRecord().bc);
    }
  }

  int nt = 0;
  for (auto& track : tracks) {

    float mEvTime = evtime.mEventTime;
    float mEvTimeRes = evtime.mEventTimeError;
    const auto mMultiplicity = evtime.mEventTimeMultiplicity;

    if (mMultiplicity > 2) {
      evtime.removeBias<MyTrack, MyFilter>(track, nt, mEvTime, mEvTimeRes);
    }

    // Delta t Pion
    float deltatpi[evTimeType::SIZEt0] = {};
    // Delta t Kaon
    float deltatka[evTimeType::SIZEt0] = {};
    // Delta t Proton
    float deltatpr[evTimeType::SIZEt0] = {};
    // Beta
    float beta[evTimeType::SIZEt0] = {};
    // Mass
    float mass[evTimeType::SIZEt0] = {};

    deltatpi[0] = track.tofSignal() - mEvTime - track.tofExpSignalPi();
    deltatka[0] = track.tofSignal() - mEvTime - track.tofExpSignalKa();
    deltatpr[0] = track.tofSignal() - mEvTime - track.tofExpSignalPr();
    beta[0] = track.getL() / (track.tofSignal() - mEvTime) * cinv;
    mass[0] = track.getP() / beta[0] * TMath::Sqrt(TMath::Abs(1 - beta[0] * beta[0]));
    // Use FT0 times
    if (mUseFT0) {
      for (int j = 1; j < evTimeType::SIZEt0; j++) {
        deltatpi[j] = track.tofSignal() - FT0evTimes[j - 1] - track.tofExpSignalPi();
        deltatka[j] = track.tofSignal() - FT0evTimes[j - 1] - track.tofExpSignalKa();
        deltatpr[j] = track.tofSignal() - FT0evTimes[j - 1] - track.tofExpSignalPr();
        beta[j] = track.getL() / (track.tofSignal() - FT0evTimes[j - 1]) * cinv;
        mass[j] = track.getP() / beta[j] * TMath::Sqrt(TMath::Abs(1 - beta[j] * beta[j]));
      }
    }

    for (int i = 0; i < trackType::SIZE; ++i) {
      if (track.source == i) {
        for (int j = 0; j < evTimeType::SIZEt0; ++j) {
          mHistDeltatPi[i][j]->Fill(deltatpi[j]);
          mHistDeltatKa[i][j]->Fill(deltatka[j]);
          mHistDeltatPr[i][j]->Fill(deltatpr[j]);
          mHistDeltatPiPt[i][j]->Fill(track.getPt(), deltatpi[j]);
          mHistDeltatKaPt[i][j]->Fill(track.getPt(), deltatka[j]);
          mHistDeltatPrPt[i][j]->Fill(track.getPt(), deltatpr[j]);
          mHistMass[i][j]->Fill(mass[j]);
          mHistBetavsP[i][j]->Fill(track.getP(), beta[j]);
          mHistMassvsP[i][j]->Fill(track.getP(), mass[j]);
        }
        if (track.getPt() > 1.5 && track.getPt() < 1.6) {
          mHistDeltatPiEvTimeRes[i]->Fill(mEvTimeRes, deltatpi[0]);
          if (mMultiplicity < 200) {
            mHistDeltatPiEvTimeMult[i]->Fill(mMultiplicity, deltatpi[0]);
          } else {
            mHistDeltatPiEvTimeMult[i]->Fill(199, deltatpi[0]);
          }
        }
      }
    }

    mHistEvTimeTOF->Fill(mEvTime_BC);
    if (track.getPt() > 1.5 && track.getPt() < 1.6) {
      mHistEvTimeResEvTimeMult->Fill(mMultiplicity, mEvTimeRes);
    }
  }
}

//__________________________________________________________

bool TaskFT0TOF::selectTrack(o2::tpc::TrackTPC const& track)
{
  if (track.getPt() < mMinPtCut) {
    return false;
  }
  if (std::abs(track.getEta()) > mEtaCut) {
    return false;
  }
  if (track.getNClusters() < mNTPCClustersCut) {
    return false;
  }

  math_utils::Point3D<float> v{};
  std::array<float, 2> dca;
  if (!(const_cast<o2::tpc::TrackTPC&>(track).propagateParamToDCA(v, mBz, &dca, mMinDCAtoBeamPipeCut)) || std::abs(dca[0]) > mMinDCAtoBeamPipeCutY) {
    return false;
  }

  return true;
}

} // namespace o2::quality_control_modules::pid
