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
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <ReconstructionDataFormats/GlobalFwdTrack.h>
#include <gsl/span>

namespace o2::quality_control_modules::muon
{

TracksTask::TracksTask()
{
}

TracksTask::~TracksTask() = default;

bool TracksTask::getBooleanParam(const char* paramName) const
{
  if (auto param = mCustomParameters.find(paramName); param != mCustomParameters.end()) {
    std::string p = param->second;
    std::transform(p.begin(), p.end(), p.begin(), ::toupper);
    return p == "TRUE";
  }
  return false;
}

void TracksTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Info, Support) << "initialize TracksTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  double maxTracksPerTF = 400;

  if (auto param = mCustomParameters.find("maxTracksPerTF"); param != mCustomParameters.end()) {
    maxTracksPerTF = std::stof(param->second);
  }

  ILOG(Info, Support) << "loading sources" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  // For track type selection
  if (auto param = mCustomParameters.find("GID"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "[TOTO] Custom parameter - GID (= sources by user): " << param->second << ENDM;
    ILOG(Info, Devel) << "[TOTO] Allowed Sources  = " << mAllowedSources << ENDM;
    mSrc = mAllowedSources & GID::getSourcesMask(param->second);
    ILOG(Info, Devel) << "Final requested sources = " << mSrc << ENDM;
  }

  ILOG(Info, Devel) << "[TOTO] Debug: Will do DataRequest" << ENDM;
  auto srcFixed = mSrc;
  if (srcFixed[GID::Source::MFTMCHMID] == 1) {
    srcFixed.reset(GID::Source::MFTMCHMID);
    srcFixed.set(GID::Source::MFTMCH);
  }
  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(srcFixed, false);

  auto createPlotter = [&](GID::Source source, std::string path) {
    if (mSrc[source] == 1) {
      std::cout << "[TOTO] Creating plotter for path " << path << std::endl;
      mTrackPlotters[source] = std::make_unique<muon::TrackPlotter>(maxTracksPerTF, source, path);
      mTrackPlotters[source]->publish(getObjectsManager());
    }
  };

  createPlotter(GID::Source::MCH, "MCH/");
  createPlotter(GID::Source::MCHMID, "MCH-MID/");
  createPlotter(GID::Source::MFTMCH, "MFT-MCH/");
  createPlotter(GID::Source::MFTMCHMID, "MFT-MCH-MID/");
}

void TracksTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity << ENDM;
}

void TracksTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
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
  if (!ctx.inputs().isValid("mchtrackdigits")) {
    ILOG(Info, Support) << "no mch track digits available on input" << ENDM;
    return false;
  }
  if (mSrc[GID::Source::MCHMID] == 1) {
    if (!ctx.inputs().isValid("matchMCHMID")) {
      ILOG(Info, Support) << "no muon (mch+mid) track available on input" << ENDM;
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
  ILOG(Info, Devel) << "Debug: MonitorData" << ENDM;

  mRecoCont.collectData(ctx, *mDataRequest.get());

  ILOG(Info, Devel) << "Debug: Collected data" << ENDM;

  if (!assertInputs(ctx)) {
    return;
  }

  ILOG(Info, Devel) << "Debug: Asserted inputs" << ENDM;

  auto tracks = mRecoCont.getMCHTracks();
  auto rofs = mRecoCont.getMCHTracksROFRecords();
  auto clusters = mRecoCont.getMCHTrackClusters();
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("mchtrackdigits");

  if (mSrc[GID::MCH] == 1) {
    ILOG(Info, Devel) << "Debug: MCH requested" << ENDM;
    if (true || mRecoCont.isTrackSourceLoaded(GID::MCH)) {
      ILOG(Info, Devel) << "Debug: MCH source loaded" << ENDM;
      mTrackPlotters[GID::MCH]->fillHistograms(mRecoCont);
    }
  }
  if (mSrc[GID::MCHMID] == 1) {
    ILOG(Info, Devel) << "Debug: MCHMID requested" << ENDM;
    if (true || mRecoCont.isMatchSourceLoaded(GID::MCHMID)) {
      ILOG(Info, Devel) << "Debug: MCHMID source loaded" << ENDM;
      mTrackPlotters[GID::MCHMID]->fillHistograms(mRecoCont);
    }
  }
  if (mSrc[GID::MFTMCH] == 1) {
    ILOG(Info, Devel) << "Debug: MFTMCH requested" << ENDM;
    if (true || mRecoCont.isTrackSourceLoaded(GID::MFTMCH)) {
      mTrackPlotters[GID::MFTMCH]->fillHistograms(mRecoCont);
    }
  }
  if (mSrc[GID::MFTMCHMID] == 1) {
    ILOG(Info, Devel) << "Debug: MFTMCHMID requested" << ENDM;
    if (true || mRecoCont.isTrackSourceLoaded(GID::MFTMCH)) {
      mTrackPlotters[GID::MFTMCHMID]->fillHistograms(mRecoCont);
    }
  }
}

void TracksTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void TracksTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TracksTask::reset()
{
  ILOG(Info, Support) << "reset" << ENDM;
  for (auto& p : mTrackPlotters) {
    p.second->reset();
  }
}

} // namespace o2::quality_control_modules::muon
