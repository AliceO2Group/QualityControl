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
/// \author Chiara Zampolli
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "GLO/ITSTPCMatchingTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

namespace o2::quality_control_modules::glo
{

ITSTPCMatchingTask::~ITSTPCMatchingTask()
{
  //  mMatchITSTPCQC.deleteHistograms();
}
void ITSTPCMatchingTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ITSTPCMatchingTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  if (auto param = mCustomParameters.find("isMC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - isMC (= use of MC info): " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mMatchITSTPCQC.setUseMC(true);
    }
  }
  if (auto param = mCustomParameters.find("minPtCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minPtCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setPtCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("EtaCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - EtaCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setEtaCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minNTPCClustersCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minNTPCClustersCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinNTPCClustersCut(atoi(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minDCACut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinDCAtoBeamPipeDistanceCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minDCACutY"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minDCACutY (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinDCAtoBeamPipeYCut(atof(param->second.c_str()));
  }

  mMatchITSTPCQC.initDataRequest();
  mMatchITSTPCQC.init();

  // Pt
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPt());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtTPC());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatch());
  // Phi
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhi());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiTPC());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhi());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhiVsPt());
  // Eta
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEta());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaTPC());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchEta());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchEtaVsPt());

  // Residuals
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoResidualPt());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoResidualPhi());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoResidualEta());

  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoChi2Matching());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoChi2Refit());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoTimeResVsPt());
  if (mMatchITSTPCQC.getUseMC()) {
    // Pt
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtPhysPrim());
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtTPCPhysPrim());
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhysPrim());
    // Phi
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiPhysPrim());
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiTPCPhysPrim());
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhiPhysPrim());
    // Eta
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaPhysPrim());
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaTPCPhysPrim());
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchEtaPhysPrim());
  }
}

void ITSTPCMatchingTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mMatchITSTPCQC.reset();
}

void ITSTPCMatchingTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ITSTPCMatchingTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mMatchITSTPCQC.run(ctx);
}

void ITSTPCMatchingTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  mMatchITSTPCQC.finalize();
}

void ITSTPCMatchingTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSTPCMatchingTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histogramss" << ENDM;
  mMatchITSTPCQC.reset();
}

} // namespace o2::quality_control_modules::glo
