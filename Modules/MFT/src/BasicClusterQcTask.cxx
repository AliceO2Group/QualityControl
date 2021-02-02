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
/// \file   BasicClusterQcTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
///

// ROOT
#include <TCanvas.h>
#include <TH1.h>
// O2
#include <DataFormatsITSMFT/Cluster.h>
#include <DataFormatsITSMFT/CompCluster.h>
#include <Framework/InputRecord.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/BasicClusterQcTask.h"

namespace o2::quality_control_modules::mft
{

BasicClusterQcTask::~BasicClusterQcTask()
{
  /*
    not needed for unique pointers
  */
}

void BasicClusterQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicClusterQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mMFT_ClusterSensorID_H = std::make_unique<TH1F>("mMFT_ClusterSensorID_H", "mMFT_ClusterSensorID_H", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mMFT_ClusterSensorID_H.get());
  getObjectsManager()->addMetadata(mMFT_ClusterSensorID_H->GetName(), "custom", "34");

  mMFT_ClusterPatternID_H = std::make_unique<TH1F>("mMFT_ClusterPatternID_H", "mMFT_ClusterPatternID_H", 2048, -0.5, 2047.5);
  getObjectsManager()->startPublishing(mMFT_ClusterPatternID_H.get());
  getObjectsManager()->addMetadata(mMFT_ClusterPatternID_H->GetName(), "custom", "34");
}

void BasicClusterQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mMFT_ClusterSensorID_H->Reset();
  mMFT_ClusterPatternID_H->Reset();
}

void BasicClusterQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void BasicClusterQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the clusters
  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("randomcluster");
  if (clusters.size() < 1)
    return;
  // fill the histograms
  for (auto& one_cluster : clusters) {
    mMFT_ClusterSensorID_H->Fill(one_cluster.getSensorID());
    mMFT_ClusterPatternID_H->Fill(one_cluster.getPatternID());
  }
}

void BasicClusterQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicClusterQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicClusterQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mMFT_ClusterSensorID_H->Reset();
  mMFT_ClusterPatternID_H->Reset();

}

} // namespace o2::quality_control_modules::mft
