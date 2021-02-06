// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
#include "DataFormatsTPC/ClusterNative.h"
#include "TPCBase/Painter.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/Clusters.h"
#include "TPC/Utility.h"

namespace o2::quality_control_modules::tpc
{

Clusters::Clusters() : TaskInterface()
{
  mWrapperVector.emplace_back(&mQCClusters.getNClusters());
  mWrapperVector.emplace_back(&mQCClusters.getQMax());
  mWrapperVector.emplace_back(&mQCClusters.getQTot());
  mWrapperVector.emplace_back(&mQCClusters.getSigmaTime());
  mWrapperVector.emplace_back(&mQCClusters.getSigmaPad());
  mWrapperVector.emplace_back(&mQCClusters.getTimeBin());
}

void Clusters::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize TPC Clusters QC task" << AliceO2::InfoLogger::InfoLogger::endm;

  addAndPublish(getObjectsManager(), mNClustersCanvasVec, { "c_Sides_N_Clusters", "c_ROCs_N_Clusters_1D", "c_ROCs_N_Clusters_2D" });
  addAndPublish(getObjectsManager(), mQMaxCanvasVec, { "c_Sides_Q_Max", "c_ROCs_Q_Max_1D", "c_ROCs_Q_Max_2D" });
  addAndPublish(getObjectsManager(), mQTotCanvasVec, { "c_Sides_Q_Tot", "c_ROCs_Q_Tot_1D", "c_ROCs_Q_Tot_2D" });
  addAndPublish(getObjectsManager(), mSigmaTimeCanvasVec, { "c_Sides_Sigma_Time", "c_ROCs_Sigma_Time_1D", "c_ROCs_Sigma_Time_2D" });
  addAndPublish(getObjectsManager(), mSigmaPadCanvasVec, { "c_Sides_Sigma_Pad", "c_ROCs_Sigma_Pad_1D", "c_ROCs_Sigma_Pad_2D" });
  addAndPublish(getObjectsManager(), mTimeBinCanvasVec, { "c_Sides_Time_Bin", "c_ROCs_Time_Bin_1D", "c_ROCs_Time_Bin_2D" });

  for (auto& wrapper : mWrapperVector) {
    getObjectsManager()->startPublishing(&wrapper);
    getObjectsManager()->addMetadata(wrapper.getObj()->getName().data(), "custom", "87");
  }
}

void Clusters::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void Clusters::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void Clusters::monitorData(o2::framework::ProcessingContext& ctx)
{
  o2::tpc::ClusterNativeAccess clusterIndex = clusterHandler(ctx.inputs());

  for (int isector = 0; isector < o2::tpc::constants::MAXSECTOR; ++isector) {
    for (int irow = 0; irow < o2::tpc::constants::MAXGLOBALPADROW; ++irow) {
      const int nClusters = clusterIndex.nClusters[isector][irow];
      for (int icl = 0; icl < nClusters; ++icl) {
        const auto& cl = *(clusterIndex.clusters[isector][irow] + icl);
        mQCClusters.processCluster(cl, o2::tpc::Sector(isector), irow);
      }
    }
  }

  mQCClusters.analyse();

  auto vecPtrNClusters = toVector(mNClustersCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mQCClusters.getNClusters(), 300, 0, 0, true, &vecPtrNClusters);

  auto vecPtrQMax = toVector(mQMaxCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mQCClusters.getQMax(), 300, 0, 0, true, &vecPtrQMax);

  auto vecPtrQTot = toVector(mQTotCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mQCClusters.getQTot(), 300, 0, 0, true, &vecPtrQTot);

  auto vecPtrSigmaTime = toVector(mSigmaTimeCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mQCClusters.getSigmaTime(), 300, 0, 0, true, &vecPtrSigmaTime);

  auto vecPtrSigmaPad = toVector(mSigmaPadCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mQCClusters.getSigmaPad(), 300, 0, 0, true, &vecPtrSigmaPad);

  auto vecPtrTimeBin = toVector(mTimeBinCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mQCClusters.getTimeBin(), 300, 0, 0, true, &vecPtrTimeBin);
}

void Clusters::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void Clusters::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void Clusters::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace o2::quality_control_modules::tpc
