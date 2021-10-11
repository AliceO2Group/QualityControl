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
#include "TPCBase/Painter.h"
#include "Framework/InputRecordWalker.h"

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
  auto& qcClusters = mQCClusters.getClusters();
  mWrapperVector.emplace_back(&qcClusters.getNClusters());
  mWrapperVector.emplace_back(&qcClusters.getQMax());
  mWrapperVector.emplace_back(&qcClusters.getQTot());
  mWrapperVector.emplace_back(&qcClusters.getSigmaTime());
  mWrapperVector.emplace_back(&qcClusters.getSigmaPad());
  mWrapperVector.emplace_back(&qcClusters.getTimeBin());
}

void Clusters::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TPC Clusters QC task" << ENDM;

  //addAndPublish(getObjectsManager(), mNClustersCanvasVec, { "c_Sides_N_Clusters", "c_ROCs_N_Clusters_1D", "c_ROCs_N_Clusters_2D" });
  //addAndPublish(getObjectsManager(), mQMaxCanvasVec, { "c_Sides_Q_Max", "c_ROCs_Q_Max_1D", "c_ROCs_Q_Max_2D" });
  //addAndPublish(getObjectsManager(), mQTotCanvasVec, { "c_Sides_Q_Tot", "c_ROCs_Q_Tot_1D", "c_ROCs_Q_Tot_2D" });
  //addAndPublish(getObjectsManager(), mSigmaTimeCanvasVec, { "c_Sides_Sigma_Time", "c_ROCs_Sigma_Time_1D", "c_ROCs_Sigma_Time_2D" });
  //addAndPublish(getObjectsManager(), mSigmaPadCanvasVec, { "c_Sides_Sigma_Pad", "c_ROCs_Sigma_Pad_1D", "c_ROCs_Sigma_Pad_2D" });
  //addAndPublish(getObjectsManager(), mTimeBinCanvasVec, { "c_Sides_Time_Bin", "c_ROCs_Time_Bin_1D", "c_ROCs_Time_Bin_2D" });

  for (auto& wrapper : mWrapperVector) {
    getObjectsManager()->startPublishing(&wrapper);
  }

  getObjectsManager()->startPublishing(&mQCClusters);
}

void Clusters::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void Clusters::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void Clusters::processClusterNative(o2::framework::InputRecord& inputs)
{
  o2::tpc::ClusterNativeAccess clusterIndex = clusterHandler(inputs);
  if (!clusterIndex.nClustersTotal) {
    return;
  }

  for (int isector = 0; isector < o2::tpc::constants::MAXSECTOR; ++isector) {
    for (int irow = 0; irow < o2::tpc::constants::MAXGLOBALPADROW; ++irow) {
      const int nClusters = clusterIndex.nClusters[isector][irow];
      for (int icl = 0; icl < nClusters; ++icl) {
        const auto& cl = *(clusterIndex.clusters[isector][irow] + icl);
        mQCClusters.getClusters().processCluster(cl, o2::tpc::Sector(isector), irow);
      }
    }
  }
}

void Clusters::processKrClusters(o2::framework::InputRecord& inputs)
{
  std::vector<InputSpec> filterKr = {
    { "krClusters", ConcreteDataTypeMatcher{ "TPC", "KRCLUSTERS" }, Lifetime::Timeframe },
    { "sampled-krClusters", ConcreteDataTypeMatcher{ "DS", "KRCLUSTERS" }, Lifetime::Timeframe },
  };

  for (auto const& inputRef : InputRecordWalker(inputs, filterKr)) {
    auto krClusters = inputs.get<gsl::span<o2::tpc::KrCluster>>(inputRef);
    for (const auto& cl : krClusters) {
      mQCClusters.getClusters().processCluster(cl, o2::tpc::Sector(cl.sector), int(cl.meanRow));
    }
  }
}

void Clusters::monitorData(o2::framework::ProcessingContext& ctx)
{
  mQCClusters.getClusters().denormalize();

  processClusterNative(ctx.inputs());
  processKrClusters(ctx.inputs());

  mQCClusters.normalize();

  //fillCanvases(mQCClusters.getNClusters(), mNClustersCanvasVec, mCustomParameters, "NClusters");
  //fillCanvases(mQCClusters.getQMax(), mQMaxCanvasVec, mCustomParameters, "Qmax");
  //fillCanvases(mQCClusters.getQTot(), mQTotCanvasVec, mCustomParameters, "Qtot");
  //fillCanvases(mQCClusters.getSigmaTime(), mSigmaTimeCanvasVec, mCustomParameters, "SigmaPad");
  //fillCanvases(mQCClusters.getSigmaPad(), mSigmaPadCanvasVec, mCustomParameters, "SigmaTime");
  //fillCanvases(mQCClusters.getTimeBin(), mTimeBinCanvasVec, mCustomParameters, "TimeBin");
}

void Clusters::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void Clusters::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void Clusters::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the canvases" << ENDM;

  mQCClusters.getClusters().reset();

  //clearCanvases(mNClustersCanvasVec);
  //clearCanvases(mQMaxCanvasVec);
  //clearCanvases(mQTotCanvasVec);
  //clearCanvases(mSigmaTimeCanvasVec);
  //clearCanvases(mSigmaPadCanvasVec);
  //clearCanvases(mTimeBinCanvasVec);
}

} // namespace o2::quality_control_modules::tpc
