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
/// \file   Clusters.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "Framework/ProcessingContext.h"
#include "DataFormatsTPC/KrCluster.h"
#include "DataFormatsTPC/ClusterNative.h"
#include "Framework/InputRecordWalker.h"
#include "DetectorsBase/GRPGeomHelper.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/Clusters.h"
#include "TPC/Utility.h"

using namespace o2::framework;
using namespace o2::tpc;

namespace o2::quality_control_modules::tpc
{

Clusters::Clusters() : TaskInterface()
{
}

void Clusters::initialize(InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC Clusters QC task" << ENDM;

  mQCClusters.setName("ClusterData");

  mNHBFPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();

  const auto last = mCustomParameters.end();
  const auto itMergeable = mCustomParameters.find("mergeableOutput");
  std::string mergeable;

  if (itMergeable == last) {
    LOGP(warning, "missing parameter 'mergeableOutput'");
    LOGP(warning, "Please add 'mergeableOutput': '<value>' to the 'taskParameters'.");
  } else {
    mergeable = itMergeable->second;
  }

  if (mergeable == "true") {
    mIsMergeable = true;
    ILOG(Info, Support) << "Using mergeable output for Clusters Task." << ENDM;
  } else if (mergeable == "false") {
    mIsMergeable = false;
    ILOG(Info, Support) << "Using non-mergeable output for Clusters Task." << ENDM;
  } else {
    mIsMergeable = false;
    LOGP(warning, "No valid value for 'mergeableOutput'. Set it as 'true' or 'false'. Falling back to non-mergeable output.");
  }

  if (mIsMergeable) {
    getObjectsManager()->startPublishing(&mQCClusters);
  } else {
    mWrapperVector.emplace_back(&mQCClusters.getClusters().getNClusters());
    mWrapperVector.emplace_back(&mQCClusters.getClusters().getQMax());
    mWrapperVector.emplace_back(&mQCClusters.getClusters().getQTot());
    mWrapperVector.emplace_back(&mQCClusters.getClusters().getSigmaTime());
    mWrapperVector.emplace_back(&mQCClusters.getClusters().getSigmaPad());
    mWrapperVector.emplace_back(&mQCClusters.getClusters().getTimeBin());
    auto occupancy = mQCClusters.getClusters().getOccupancy(mNHBFPerTF);
    mWrapperVector.emplace_back(&occupancy);

    addAndPublish(getObjectsManager(), mNClustersCanvasVec, { "c_Sides_N_Clusters", "c_ROCs_N_Clusters_1D", "c_ROCs_N_Clusters_2D" });
    addAndPublish(getObjectsManager(), mQMaxCanvasVec, { "c_Sides_Q_Max", "c_ROCs_Q_Max_1D", "c_ROCs_Q_Max_2D" });
    addAndPublish(getObjectsManager(), mQTotCanvasVec, { "c_Sides_Q_Tot", "c_ROCs_Q_Tot_1D", "c_ROCs_Q_Tot_2D" });
    addAndPublish(getObjectsManager(), mSigmaTimeCanvasVec, { "c_Sides_Sigma_Time", "c_ROCs_Sigma_Time_1D", "c_ROCs_Sigma_Time_2D" });
    addAndPublish(getObjectsManager(), mSigmaPadCanvasVec, { "c_Sides_Sigma_Pad", "c_ROCs_Sigma_Pad_1D", "c_ROCs_Sigma_Pad_2D" });
    addAndPublish(getObjectsManager(), mTimeBinCanvasVec, { "c_Sides_Time_Bin", "c_ROCs_Time_Bin_1D", "c_ROCs_Time_Bin_2D" });
    addAndPublish(getObjectsManager(), mOccupancyCanvasVec, { "c_Sides_Occupancy", "c_ROCs_Occupancy_1D", "c_ROCs_Occupancy_2D" });

    for (auto& wrapper : mWrapperVector) {
      getObjectsManager()->startPublishing<true>(&wrapper);
    }
  }
}

void Clusters::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  mQCClusters.getClusters().reset();
  if (!mIsMergeable) {
    clearCanvases(mNClustersCanvasVec);
    clearCanvases(mQMaxCanvasVec);
    clearCanvases(mQTotCanvasVec);
    clearCanvases(mSigmaTimeCanvasVec);
    clearCanvases(mSigmaPadCanvasVec);
    clearCanvases(mTimeBinCanvasVec);
  }
}

void Clusters::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void Clusters::processClusterNative(InputRecord& inputs)
{
  const auto& inputsTPCclusters = clusterHandler(inputs);
  if (!inputsTPCclusters->clusterIndex.nClustersTotal) {
    return;
  }

  for (int isector = 0; isector < o2::tpc::constants::MAXSECTOR; ++isector) {
    for (int irow = 0; irow < o2::tpc::constants::MAXGLOBALPADROW; ++irow) {
      const int nClusters = inputsTPCclusters->clusterIndex.nClusters[isector][irow];
      for (int icl = 0; icl < nClusters; ++icl) {
        const auto& cl = *(inputsTPCclusters->clusterIndex.clusters[isector][irow] + icl);
        mQCClusters.getClusters().processCluster(cl, Sector(isector), irow);
      }
    }
  }
  mQCClusters.getClusters().endTF();
}

void Clusters::processKrClusters(InputRecord& inputs)
{
  std::vector<InputSpec> filterKr = {
    { "krClusters", ConcreteDataTypeMatcher{ "TPC", "KRCLUSTERS" }, Lifetime::Timeframe },
    { "sampled-krClusters", ConcreteDataTypeMatcher{ "DS", "KRCLUSTERS" }, Lifetime::Timeframe },
  };

  for (auto const& inputRef : InputRecordWalker(inputs, filterKr)) {
    auto krClusters = inputs.get<gsl::span<KrCluster>>(inputRef);
    for (const auto& cl : krClusters) {
      mQCClusters.getClusters().processCluster(cl, Sector(cl.sector), int(cl.meanRow));
    }
  }
  mQCClusters.getClusters().endTF();
}

void Clusters::monitorData(ProcessingContext& ctx)
{
  mQCClusters.getClusters().denormalize();

  processClusterNative(ctx.inputs());
  processKrClusters(ctx.inputs());

  if (!mIsMergeable) {
    mQCClusters.getClusters().normalize();

    fillCanvases(mQCClusters.getClusters().getNClusters(), mNClustersCanvasVec, mCustomParameters, "NClusters");
    fillCanvases(mQCClusters.getClusters().getQMax(), mQMaxCanvasVec, mCustomParameters, "Qmax");
    fillCanvases(mQCClusters.getClusters().getQTot(), mQTotCanvasVec, mCustomParameters, "Qtot");
    fillCanvases(mQCClusters.getClusters().getSigmaTime(), mSigmaTimeCanvasVec, mCustomParameters, "SigmaPad");
    fillCanvases(mQCClusters.getClusters().getSigmaPad(), mSigmaPadCanvasVec, mCustomParameters, "SigmaTime");
    fillCanvases(mQCClusters.getClusters().getTimeBin(), mTimeBinCanvasVec, mCustomParameters, "TimeBin");
    fillCanvases(mQCClusters.getClusters().getTimeBin(), mOccupancyCanvasVec, mCustomParameters, "Occupancy");
  }
}

void Clusters::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  ILOG(Info, Support) << "Processed TFs: " << mQCClusters.getClusters().getProcessedTFs() << ENDM;

  if (mIsMergeable) {
    mQCClusters.getClusters().normalize();
  }
}

void Clusters::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void Clusters::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the data" << ENDM;

  mQCClusters.getClusters().reset();

  if (!mIsMergeable) {
    clearCanvases(mNClustersCanvasVec);
    clearCanvases(mQMaxCanvasVec);
    clearCanvases(mQTotCanvasVec);
    clearCanvases(mSigmaTimeCanvasVec);
    clearCanvases(mSigmaPadCanvasVec);
    clearCanvases(mTimeBinCanvasVec);
    clearCanvases(mOccupancyCanvasVec);
  }
}

} // namespace o2::quality_control_modules::tpc
