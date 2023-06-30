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
#include "Common/Utils.h"

namespace o2::quality_control_modules::tpc
{

PID::PID() : TaskInterface() {}

PID::~PID()
{
}

void PID::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC PID QC task" << ENDM;
  // elementary cuts for PID from json file
  const int cutMinNCluster = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "cutMinNCluster");
  const float cutAbsTgl = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutAbsTgl");
  const float cutMindEdxTot = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMindEdxTot");
  const float cutMaxdEdxTot = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMaxdEdxTot");
  const float cutMinpTPC = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMinpTPC");
  const float cutMaxpTPC = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMaxpTPC");
  const float cutMinpTPCMIPs = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMinpTPCMIPs");
  const float cutMaxpTPCMIPs = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMaxpTPCMIPs");

  // set track cutss defaults are (AbsEta = 1.0, nCluster = 60, MindEdxTot  = 20)
  mQCPID.setPIDCuts(cutMinNCluster, cutAbsTgl, cutMindEdxTot, cutMaxdEdxTot, cutMinpTPC, cutMaxpTPC, cutMinpTPCMIPs, cutMaxpTPCMIPs);
  mQCPID.initializeHistograms();
  // pass map of vectors of histograms to be beutified!
  o2::tpc::qc::helpers::setStyleHistogramsInMap(mQCPID.getMapOfHisto());
  for (auto const& pair : mQCPID.getMapOfHisto()) {
    for (auto& hist : pair.second) {
      getObjectsManager()->startPublishing(hist.get());
    }
  }
}

void PID::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  mQCPID.resetHistograms();
}

void PID::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void PID::monitorData(o2::framework::ProcessingContext& ctx)
{
  using TrackType = std::vector<o2::tpc::TrackTPC>;
  auto tracks = ctx.inputs().get<TrackType>("inputTracks");
  // ILOG(Info, Support) << "monitorData: " << tracks.size() << ENDM;

  for (auto const& track : tracks) {
    mQCPID.processTrack(track);
  }
}

void PID::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void PID::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void PID::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mQCPID.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
