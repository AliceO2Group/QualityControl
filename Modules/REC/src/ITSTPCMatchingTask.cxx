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
/// \file   ITSTPCMatchingTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "REC/ITSTPCMatchingTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

namespace o2::quality_control_modules::rec
{

ITSTPCMatchingTask::~ITSTPCMatchingTask()
{
  //  mMatchITSTPCQC.deleteHistograms();
}

void ITSTPCMatchingTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize ITSTPCMatchingTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  if (auto param = mCustomParameters.find("isMC"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - isMC (= use of MC info): " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mMatchITSTPCQC.setUseMC(true);
    }
  }
  if (auto param = mCustomParameters.find("minPtCut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minPtCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setPtCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("EtaCut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - EtaCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setEtaCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minNTPCClustersCut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minNTPCClustersCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinNTPCClustersCut(atoi(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACut"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minDCACut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinDCAtoBeamPipeDistanceCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACutY"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - minDCACutY (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinDCAtoBeamPipeYCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("grpFileName"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - GRP filename: " << param->second << ENDM;
    mMatchITSTPCQC.setGRPFileName(param->second);
  }
  if (auto param = mCustomParameters.find("geomFileName"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - geometry filename: " << param->second << ENDM;
    mMatchITSTPCQC.setGeomFileName(param->second);
  }

  mMatchITSTPCQC.init();
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtTPC());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoFractionITSTPCmatch());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPt());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEta());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoChi2Matching());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoChi2Refit());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoTimeResVsPt());
}

void ITSTPCMatchingTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mMatchITSTPCQC.reset();
}

void ITSTPCMatchingTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSTPCMatchingTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mMatchITSTPCQC.run(ctx);
}

void ITSTPCMatchingTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  mMatchITSTPCQC.finalize();
}

void ITSTPCMatchingTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSTPCMatchingTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histograms" << ENDM;
  mMatchITSTPCQC.reset();
}

} // namespace o2::quality_control_modules::rec
