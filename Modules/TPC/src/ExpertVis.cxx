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
/// \file   ExpertVis.cxx
/// \author Berkin Ulukutlu & Jens Wiechula
///

// root includes
#include <THnSparse.h>

// O2 includes
#include "Framework/ProcessingContext.h"
#include "DataFormatsTPC/TrackTPC.h"
#include <Framework/InputRecord.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/ExpertVis.h"

namespace o2::quality_control_modules::tpc
{

void ExpertVis::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TPC ExpertVis QC task" << ENDM;

  mQCExpertVis.initializeHistograms();
  getObjectsManager()->startPublishing(mQCExpertVis.getHistogramPID());
  getObjectsManager()->startPublishing(mQCExpertVis.getHistogramTracks());
}


void ExpertVis::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mQCExpertVis.resetHistograms();
}

void ExpertVis::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ExpertVis::monitorData(o2::framework::ProcessingContext& ctx)
{
  using TrackType = std::vector<o2::tpc::TrackTPC>;
  auto tracks = ctx.inputs().get<TrackType>("inputTracks");
  //ILOG(Info, Support) << "monitorData: " << tracks.size() << ENDM;

  for (auto const& track : tracks) {
    mQCExpertVis.processTrack(track);
  }
}

void ExpertVis::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ExpertVis::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ExpertVis::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mQCExpertVis.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
