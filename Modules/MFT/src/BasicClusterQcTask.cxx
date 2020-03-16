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
  ILOG(Info) << "initialize BasicClusterQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mMFT_nPix_H = std::make_unique<TH1F>("mMFT_nPix", "mMFT_nPix", 50, -0.5, 49.5);
  getObjectsManager()->startPublishing(mMFT_nPix_H.get());
  getObjectsManager()->addMetadata(mMFT_nPix_H->GetName(), "custom", "34");
}

void BasicClusterQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  mMFT_nPix_H->Reset();
}

void BasicClusterQcTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void BasicClusterQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the clusters
  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::Cluster>>("randomcluster");
  if (clusters.size() < 1)
    return;
  // fill the histograms
  for (auto& one_cluster : clusters) {
    mMFT_nPix_H->Fill(one_cluster.getNPix());
  }
}

void BasicClusterQcTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void BasicClusterQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void BasicClusterQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mMFT_nPix_H->Reset();
}

} // namespace o2::quality_control_modules::mft
