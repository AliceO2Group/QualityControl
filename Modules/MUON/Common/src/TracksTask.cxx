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
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <MCHGeometryTransformer/Transformations.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
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

  if (!o2::base::GeometryManager::isGeometryLoaded()) {
    TaskInterface::retrieveConditionAny<TObject>("GLO/Config/Geometry");
  }

  auto transformation = o2::mch::geo::transformationFromTGeoManager(*gGeoManager);

  double maxTracksPerTF = 400;

  if (auto param = mCustomParameters.find("maxTracksPerTF"); param != mCustomParameters.end()) {
    maxTracksPerTF = std::stof(param->second);
  }

  mUseMatchedMCHMIDTracks = getBooleanParam("matchedMCHMIDTracks");

  mTrackPlotter = std::make_unique<muon::TrackPlotter>(transformation, maxTracksPerTF);

  publish(*(mTrackPlotter.get()));
}

void TracksTask::publish(const TrackPlotter& tp)
{
  auto hinfos = tp.histograms();

  for (auto hinfo : hinfos) {
    getObjectsManager()->startPublishing(hinfo.histo);
    getObjectsManager()->setDefaultDrawOptions(hinfo.histo, hinfo.drawOptions);
    getObjectsManager()->setDisplayHint(hinfo.histo, hinfo.displayHints);
  }
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
  if (!ctx.inputs().isValid("mchtracks")) {
    ILOG(Info, Support) << "no mch tracks available on input" << ENDM;
    return false;
  }
  if (!ctx.inputs().isValid("mchtrackrofs")) {
    ILOG(Info, Support) << "no mch track rofs available on input" << ENDM;
    return false;
  }
  if (!ctx.inputs().isValid("mchtrackclusters")) {
    ILOG(Info, Support) << "no mch track clusters available on input" << ENDM;
    return false;
  }
  if (!ctx.inputs().isValid("mchtrackdigits")) {
    ILOG(Info, Support) << "no mch track digits available on input" << ENDM;
    return false;
  }
  if (mUseMatchedMCHMIDTracks) {
    if (!ctx.inputs().isValid("muontracks")) {
      ILOG(Info, Support) << "no muon (mch+mid) track available on input" << ENDM;
      return false;
    }
  }
  return true;
}

void TracksTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  if (!assertInputs(ctx)) {
    return;
  }

  auto tracks = ctx.inputs().get<gsl::span<o2::mch::TrackMCH>>("mchtracks");
  auto rofs = ctx.inputs().get<gsl::span<o2::mch::ROFRecord>>("mchtrackrofs");
  auto clusters = ctx.inputs().get<gsl::span<o2::mch::Cluster>>("mchtrackclusters");
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("mchtrackdigits");

  if (mUseMatchedMCHMIDTracks) {
    auto muonTracks = ctx.inputs().get<gsl::span<o2::dataformats::TrackMCHMID>>("muontracks");
    mTrackPlotter->fillHistograms(rofs, tracks, clusters, digits, muonTracks);
  } else {
    mTrackPlotter->fillHistograms(rofs, tracks, clusters, digits);
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
  ILOG(Warning, Support) << "resetting the histograms is not implemented" << ENDM;
}

} // namespace o2::quality_control_modules::muon
