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

#include "MUONCommon/TracksTask.h"

#include "MUONCommon/Helpers.h"
#include "QualityControl/ObjectsManager.h"
#include <DataFormatsMCH/Cluster.h>
#include <DataFormatsMCH/Digit.h>
#include <DataFormatsMCH/ROFRecord.h>
#include <DataFormatsMCH/TrackMCH.h>
#include <DetectorsBase/GeometryManager.h>
#include "QualityControl/QcInfoLogger.h"
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <Framework/TimingInfo.h>
#include <MCHGeometryTransformer/Transformations.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <ReconstructionDataFormats/GlobalFwdTrack.h>
#include <gsl/span>

#include <fstream>

namespace o2::quality_control_modules::muon
{

TracksTask::TracksTask()
{
}

TracksTask::~TracksTask() = default;

GID::mask_t adaptSource(GID::mask_t src)
{
  if (src[GID::Source::MFTMCHMID] == 1) {
    src.reset(GID::Source::MFTMCHMID); // does not exist
    src.set(GID::Source::MFTMCH);
    // ensure we request the individual tracks as we use their information in the plotter
    src.set(GID::Source::MFT);
    src.set(GID::Source::MCH);
    src.set(GID::Source::MID);
  }
  if (src[GID::Source::MFTMCH] == 1) {
    // ensure we request the individual tracks as we use their information in the plotter
    src.set(GID::Source::MFT);
    src.set(GID::Source::MCH);
  }
  if (src[GID::Source::MCHMID] == 1) {
    // ensure we request the individual tracks as we use their information in the plotter
    src.set(GID::Source::MCH);
    src.set(GID::Source::MID);
  }
  return src;
}

void TracksTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Debug, Devel) << "initialize TracksTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  ILOG(Info, Support) << "loading sources" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  auto srcFixed = mSrc;
  // For track type selection
  if (auto param = mCustomParameters.find("GID"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - GID (= sources by user): " << param->second << ENDM;
    ILOG(Info, Devel) << "Allowed sources           = " << mAllowedSources << " " << GID::getSourcesNames(mAllowedSources) << ENDM;
    auto requested = GID::getSourcesMask(param->second);
    ILOG(Info, Devel) << "Requested Sources         = " << requested << " " << GID::getSourcesNames(requested) << ENDM;
    mSrc = mAllowedSources & requested;
    srcFixed = adaptSource(mSrc);
    ILOG(Info, Devel) << "Allowed requested sources = " << mSrc << " " << GID::getSourcesNames(mSrc) << ENDM;
    ILOG(Info, Devel) << "Sources for data request  = " << srcFixed << " " << GID::getSourcesNames(srcFixed) << ENDM;
  }

  ILOG(Info, Support) << "Will do DataRequest for " << GID::getSourcesNames(srcFixed) << ENDM;
  if (srcFixed[GID::Source::MFTMCHMID] == 1) {
    srcFixed.reset(GID::Source::MFTMCHMID);
    srcFixed.set(GID::Source::MFTMCH);
  }
  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(srcFixed, false);
}

void TracksTask::createTrackHistos(const Activity& activity)
{
  bool fullHistos = getConfigurationParameter<bool>(mCustomParameters, "fullHistos", false, activity);

  double maxTracksPerTF = getConfigurationParameter<double>(mCustomParameters, "maxTracksPerTF", 400, activity);
  double cutRAbsMin = getConfigurationParameter<double>(mCustomParameters, "cutRAbsMin", 17.6, activity);
  double cutRAbsMax = getConfigurationParameter<double>(mCustomParameters, "cutRAbsMax", 89.5, activity);
  double cutEtaMin = getConfigurationParameter<double>(mCustomParameters, "cutEtaMin", -4.0, activity);
  double cutEtaMax = getConfigurationParameter<double>(mCustomParameters, "cutEtaMax", -2.5, activity);
  double cutPtMin = getConfigurationParameter<double>(mCustomParameters, "cutPtMin", 0.5, activity);
  double cutChi2Min = getConfigurationParameter<double>(mCustomParameters, "cutChi2Min", 0, activity);
  double cutChi2Max = getConfigurationParameter<double>(mCustomParameters, "cutChi2Max", 1000, activity);
  double nSigmaPDCA = getConfigurationParameter<double>(mCustomParameters, "nSigmaPDCA", 6, activity);
  double matchScoreMaxMFT = getConfigurationParameter<double>(mCustomParameters, "matchScoreMaxMFT", 1000, activity);
  double matchChi2MaxMFT = getConfigurationParameter<double>(mCustomParameters, "matchChi2MaxMFT", 1000, activity);
  double diMuonTimeCut = getConfigurationParameter<double>(mCustomParameters, "diMuonTimeCut", 100, activity) / 1000;

  int etaBins = getConfigurationParameter<int>(mCustomParameters, "etaBins", 200, activity);
  int phiBins = getConfigurationParameter<int>(mCustomParameters, "phiBins", 180, activity);
  int ptBins = getConfigurationParameter<int>(mCustomParameters, "ptBins", 300, activity);

  //======================================
  // Track plotters without cuts

  auto createPlotter = [&](GID::Source source, std::string path) {
    if (mSrc[source] == 1) {
      ILOG(Info, Devel) << "Creating plotter for path " << path << ENDM;
      mTrackPlotters[source] = std::make_unique<muon::TrackPlotter>(maxTracksPerTF, etaBins, phiBins, ptBins, source, path, fullHistos);
      mTrackPlotters[source]->publish(getObjectsManager());
    }
  };

  createPlotter(GID::Source::MCH, "");
  createPlotter(GID::Source::MCHMID, "MCH-MID/");
  createPlotter(GID::Source::MFTMCH, "MFT-MCH/");
  createPlotter(GID::Source::MFTMCHMID, "MFT-MCH-MID/");

  //======================================
  // Track plotters with cuts

  std::vector<MuonCutFunc> muonCuts{
    // Rabs cut
    [cutRAbsMin, cutRAbsMax](const MuonTrack& t) { return ((t.getRAbs() >= cutRAbsMin) && (t.getRAbs() <= cutRAbsMax)); },
    // Eta cut
    [cutEtaMin, cutEtaMax](const MuonTrack& t) { return ((t.getMuonMomentumAtVertexMCH().eta() >= cutEtaMin) && (t.getMuonMomentumAtVertexMCH().eta() <= cutEtaMax)); },
    // Pt cut
    [cutPtMin](const MuonTrack& t) { return ((t.getMuonMomentumAtVertexMCH().Pt() >= cutPtMin)); },
    // pDCA cut
    [nSigmaPDCA](const MuonTrack& t) {
      static const double sigmaPDCA23 = 80.;
      static const double sigmaPDCA310 = 54.;
      static const double relPRes = 0.0004;
      static const double slopeRes = 0.0005;

      double thetaAbs = TMath::ATan(t.getRAbs() / 505.) * TMath::RadToDeg();

      double pUncorr = t.getTrackParamMCH().p();
      double p = t.getMuonMomentumAtVertexMCH().P();

      double pDCA = pUncorr * t.getDCAMCH();
      double sigmaPDCA = (thetaAbs < 3) ? sigmaPDCA23 : sigmaPDCA310;
      double nrp = nSigmaPDCA * relPRes * p;
      double pResEffect = sigmaPDCA / (1. - nrp / (1. + nrp));
      double slopeResEffect = 535. * slopeRes * p;
      double sigmaPDCAWithRes = TMath::Sqrt(pResEffect * pResEffect + slopeResEffect * slopeResEffect);
      if (pDCA > nSigmaPDCA * sigmaPDCAWithRes) {
        return false;
      }

      return true;
    },
    // MFT-MCH match score
    [matchScoreMaxMFT](const MuonTrack& t) {
      if (t.hasMFT() && t.hasMCH() && t.getMatchInfoFwd().getMFTMCHMatchingScore() > matchScoreMaxMFT)
        return false;
      return true;
    },
    // MFT-MCH chi2 score
    [matchChi2MaxMFT](const MuonTrack& t) {
      if (t.hasMFT() && t.hasMCH() && t.getMatchInfoFwd().getMFTMCHMatchingChi2() > matchChi2MaxMFT)
        return false;
      return true;
    },
    // MCH chi2 cut
    [cutChi2Min, cutChi2Max](const MuonTrack& t) { return ((t.getChi2OverNDFMCH() >= cutChi2Min) && (t.getChi2OverNDFMCH() <= cutChi2Max)); }
  };

  std::vector<DiMuonCutFunc> diMuonCuts{
    // cut on time difference between the two muon tracks
    [diMuonTimeCut](const MuonTrack& t1, const MuonTrack& t2) { return (std::abs(t1.getTime().getTimeStamp() - t2.getTime().getTimeStamp()) < diMuonTimeCut); }
  };

  auto createPlotterWithCuts = [&](GID::Source source, std::string path) {
    if (mSrc[source] == 1) {
      ILOG(Info, Devel) << "Creating plotter for path " << path << ENDM;
      mTrackPlottersWithCuts[source] = std::make_unique<muon::TrackPlotter>(maxTracksPerTF, etaBins, phiBins, ptBins, source, path, fullHistos);
      mTrackPlottersWithCuts[source]->setMuonCuts(muonCuts);
      mTrackPlottersWithCuts[source]->setDiMuonCuts(diMuonCuts);
      mTrackPlottersWithCuts[source]->publish(getObjectsManager());
    }
  };

  createPlotterWithCuts(GID::Source::MCH, "WithCuts/");
  createPlotterWithCuts(GID::Source::MCHMID, "MCH-MID/WithCuts/");
  createPlotterWithCuts(GID::Source::MFTMCH, "MFT-MCH/WithCuts/");
  createPlotterWithCuts(GID::Source::MFTMCHMID, "MFT-MCH-MID/WithCuts/");
}

void TracksTask::removeTrackHistos()
{
  ILOG(Debug, Devel) << "Un-publishing objects" << ENDM;
  for (auto& p : mTrackPlotters) {
    p.second->unpublish(getObjectsManager());
  }
  for (auto& p : mTrackPlottersWithCuts) {
    p.second->unpublish(getObjectsManager());
  }

  ILOG(Debug, Devel) << "Destroying objects" << ENDM;
  mTrackPlotters.clear();
  mTrackPlottersWithCuts.clear();
}

void TracksTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity << ENDM;
  createTrackHistos(activity);
}

void TracksTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

bool TracksTask::assertInputs(o2::framework::ProcessingContext& ctx)
{
  if (!ctx.inputs().isValid("trackMCH")) {
    ILOG(Info, Support) << "no mch tracks available on input" << ENDM;
    return false;
  }
  if (!ctx.inputs().isValid("trackMCHROF")) {
    ILOG(Info, Support) << "no mch track rofs available on input" << ENDM;
    return false;
  }
  if (!ctx.inputs().isValid("trackMCHTRACKCLUSTERS")) {
    ILOG(Info, Support) << "no mch track clusters available on input" << ENDM;
    return false;
  }
  if (mSrc[GID::Source::MCHMID] == 1) {
    if (!ctx.inputs().isValid("matchMCHMID")) {
      ILOG(Info, Support) << "no muon (mch+mid) track available on input" << ENDM;
      return false;
    }
    if (!ctx.inputs().isValid("trackMID")) {
      ILOG(Info, Support) << "no mid track available on input" << ENDM;
      return false;
    }
  }
  if (mSrc[GID::Source::MFTMCH] == 1) {
    if (!ctx.inputs().isValid("fwdtracks")) {
      ILOG(Info, Support) << "no muon (mch+mft) track available on input" << ENDM;
      return false;
    }
  }
  if (mSrc[GID::Source::MFTMCHMID] == 1) {
    if (!ctx.inputs().isValid("fwdtracks")) {
      ILOG(Info, Support) << "no muon (mch+mft+mid) track available on input" << ENDM;
      return false;
    }
  }
  return true;
}

void TracksTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Debug, Devel) << "Debug: MonitorData" << ENDM;

  int firstTForbit = ctx.services().get<o2::framework::TimingInfo>().firstTForbit;
  ILOG(Debug, Devel) << "Debug: firstTForbit=" << firstTForbit << ENDM;

  if (!assertInputs(ctx)) {
    return;
  }

  ILOG(Debug, Devel) << "Debug: Asserted inputs" << ENDM;

  mRecoCont.collectData(ctx, *mDataRequest.get());

  ILOG(Debug, Devel) << "Debug: Collected data" << ENDM;

  for (auto& p : mTrackPlotters) {
    if (p.second) {
      p.second->setFirstTForbit(firstTForbit);
    }
  }
  for (auto& p : mTrackPlottersWithCuts) {
    if (p.second) {
      p.second->setFirstTForbit(firstTForbit);
    }
  }

  if (mSrc[GID::MCH] == 1) {
    ILOG(Debug, Devel) << "Debug: MCH requested" << ENDM;
    if (mRecoCont.isTrackSourceLoaded(GID::MCH)) {
      ILOG(Debug, Devel) << "Debug: MCH source loaded" << ENDM;
      mTrackPlotters[GID::MCH]->fillHistograms(mRecoCont);
      mTrackPlottersWithCuts[GID::MCH]->fillHistograms(mRecoCont);
    }
  }
  if (mSrc[GID::MCHMID] == 1) {
    ILOG(Debug, Devel) << "Debug: MCHMID requested" << ENDM;
    if (mRecoCont.isMatchSourceLoaded(GID::MCHMID)) {
      ILOG(Debug, Devel) << "Debug: MCHMID source loaded" << ENDM;
      mTrackPlotters[GID::MCHMID]->fillHistograms(mRecoCont);
      mTrackPlottersWithCuts[GID::MCHMID]->fillHistograms(mRecoCont);
    }
  }
  if (mSrc[GID::MFTMCH] == 1) {
    ILOG(Debug, Devel) << "Debug: MFTMCH requested" << ENDM;
    if (mRecoCont.isTrackSourceLoaded(GID::MFTMCH)) {
      mTrackPlotters[GID::MFTMCH]->fillHistograms(mRecoCont);
      mTrackPlottersWithCuts[GID::MFTMCH]->fillHistograms(mRecoCont);
    }
  }
  if (mSrc[GID::MFTMCHMID] == 1) {
    ILOG(Debug, Devel) << "Debug: MFTMCHMID requested" << ENDM;
    if (mRecoCont.isTrackSourceLoaded(GID::MFTMCH)) {
      mTrackPlotters[GID::MFTMCHMID]->fillHistograms(mRecoCont);
      mTrackPlottersWithCuts[GID::MFTMCHMID]->fillHistograms(mRecoCont);
    }
  }
}

void TracksTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  for (auto& p : mTrackPlotters) {
    p.second->endOfCycle();
  }
  for (auto& p : mTrackPlottersWithCuts) {
    p.second->endOfCycle();
  }
}

void TracksTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
  removeTrackHistos();
}

void TracksTask::reset()
{
  ILOG(Debug, Devel) << "reset" << ENDM;
  for (auto& p : mTrackPlotters) {
    p.second->reset();
  }
  for (auto& p : mTrackPlottersWithCuts) {
    p.second->reset();
  }
}

} // namespace o2::quality_control_modules::muon
