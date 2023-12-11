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
/// \file   TrackClusters.cxx
/// \author Laura Serksnyte
///

// O2 includes
#include "Framework/ProcessingContext.h"
#include <Framework/InputRecord.h>
#include "DataFormatsTPC/TrackTPC.h"
#include "TPCQC/Helpers.h"
#include "DataFormatsTPC/ClusterNative.h"
#include "DataFormatsTPC/WorkflowHelper.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/TrackClusters.h"
#include "Common/Utils.h"

using namespace o2::framework;
using namespace o2::tpc;

namespace o2::quality_control_modules::tpc
{

TrackClusters::TrackClusters() : TaskInterface()
{
}

void TrackClusters::initialize(InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC TrackClusters QC task" << ENDM;

  // do random generator
  const int seed = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "seed");
  mRandomGenerator = new TRandom3(seed);
  mSamplingFraction = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "samplingFraction");

  const int cutMinNCluster = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "cutMinNCluster");
  const float cutMindEdxTot = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMindEdxTot");
  const float cutAbsEta = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutAbsEta");

  mQCTrackClusters.setTrackClustersCuts(cutMinNCluster, cutMindEdxTot, cutAbsEta);
  mQCTrackClusters.initializeHistograms();

  o2::tpc::qc::helpers::setStyleHistogramsInMap(mQCTrackClusters.getMapOfHisto());
  for (auto const& pair : mQCTrackClusters.getMapOfHisto()) {
    for (auto& hist : pair.second) {
      getObjectsManager()->startPublishing(hist.get());
    }
  }
}

void TrackClusters::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  // serksnyte: anything neeeded for start of activity (in track tasks there is a reset of histograms)?
}

void TrackClusters::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TrackClusters::monitorData(ProcessingContext& ctx)
{
  if (mRandomGenerator->Uniform(0., 1.) < mSamplingFraction) {

    using TrackType = std::vector<o2::tpc::TrackTPC>;
    using ClusterRefType = std::vector<o2::tpc::TPCClRefElem>;

    auto tracks = ctx.inputs().get<TrackType>("inputTracks");
    const auto& inputsTPCclusters = o2::tpc::getWorkflowTPCInput(ctx, 0, false);
    auto clusRefs = ctx.inputs().get<ClusterRefType>("inputClusRefs");

    mQCTrackClusters.processTrackAndClusters(&tracks, &inputsTPCclusters->clusterIndex, &clusRefs);
  }
}

void TrackClusters::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TrackClusters::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TrackClusters::reset()
{
  ILOG(Debug, Devel) << "Resetting the data" << ENDM;
  mQCTrackClusters.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
