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
/// \file   QcMFTClusterTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
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
#include "MFT/QcMFTClusterTask.h"

namespace o2::quality_control_modules::mft
{

QcMFTClusterTask::~QcMFTClusterTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize QcMFTClusterTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mClusterSensorIndex = std::make_unique<TH1F>("mMFTClusterSensorIndex", "Chip Cluster Occupancy;Chip ID;#Entries",  936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mClusterSensorIndex.get());

  mClusterPatternIndex = std::make_unique<TH1F>("mMFTClusterPatternIndex", "Cluster Pattern ID;Pattern ID;#Entries", 300, -0.5, 299.5);
  getObjectsManager()->startPublishing(mClusterPatternIndex.get());

}

void QcMFTClusterTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mClusterSensorIndex->Reset();
  mClusterPatternIndex->Reset();
}

void QcMFTClusterTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void QcMFTClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the clusters
  const auto clusters = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("randomcluster");
  if (clusters.size() < 1)
    return;
  // fill the histograms
  for (auto& oneCluster : clusters) {
    mClusterSensorIndex->Fill(oneCluster.getSensorID() + 1);
    mClusterPatternIndex->Fill(oneCluster.getPatternID() + 1);
  }
}

void QcMFTClusterTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void QcMFTClusterTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void QcMFTClusterTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mClusterSensorIndex->Reset();
  mClusterPatternIndex->Reset();
}

} // namespace o2::quality_control_modules::mft
