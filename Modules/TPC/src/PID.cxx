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
/// \file   PID.cxx
/// \author Jens Wiechula
///

// root includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

// O2 includes
#include "Framework/ProcessingContext.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "TPCQC/Helpers.h"
#include <Framework/InputRecord.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/PID.h"

namespace o2::quality_control_modules::tpc
{

PID::PID() : TaskInterface() {}

PID::~PID()
{
}

void PID::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TPC PID QC task" << ENDM;

  mQCPID.initializeHistograms();
  //pass map of vectors of histograms to be beutified!
  o2::tpc::qc::helpers::setStyleHistogramsInMap(mQCPID.getMapOfHisto());
  for (auto const& [key, vecOfHist] : mQCPID.getMapOfHisto()) {
    for (auto& hist : vecOfHist) {
      getObjectsManager()->startPublishing(hist.get());
    }
 }
}

void PID::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mQCPID.resetHistograms();
}

void PID::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void PID::monitorData(o2::framework::ProcessingContext& ctx)
{
  using TrackType = std::vector<o2::tpc::TrackTPC>;
  auto tracks = ctx.inputs().get<TrackType>("inputTracks");
  //ILOG(Info, Support) << "monitorData: " << tracks.size() << ENDM;

  for (auto const& track : tracks) {
    mQCPID.processTrack(track);
  }
}

void PID::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void PID::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void PID::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mQCPID.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
