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

using matchType = o2::globaltracking::MatchITSTPCQC::matchType;

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

  ///////////////////////////////   Track selections for MatchITSTPCQC   ////////////////////////////////
  // ITS track
  if (auto param = mCustomParameters.find("minPtITSCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minPtITSCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinPtITSCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("etaITSCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - etaITSCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setEtaITSCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("minNITSClustersCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minNITSClustersCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinNClustersITS(atoi(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("maxChi2PerClusterITS"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - maxChi2PerClusterITS (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMaxChi2PerClusterITS(atoi(param->second.c_str()));
  }
  // TO DO: define an agreed way to implement the setter for ITS matching (min. # layers, which layers)
  // [...] --> exploit the method TrackCuts::setRequireHitsInITSLayers(...)
  // TPC track
  if (auto param = mCustomParameters.find("minPtTPCCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minPtTPCCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMinPtTPCCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("etaTPCCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - etaTPCCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setEtaTPCCut(atof(param->second.c_str()));
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
  // ITS-TPC kinematics
  if (auto param = mCustomParameters.find("minPtCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - minPtCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setPtCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("maxPtCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - maxPtCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMaxPtCut(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("etaCut"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - EtaCut (for track selection): " << param->second << ENDM;
    mMatchITSTPCQC.setEtaCut(atof(param->second.c_str()));
  }

  mMatchITSTPCQC.initDataRequest();
  mMatchITSTPCQC.init();

  for (int i = 0; i < matchType::SIZE; ++i) {
    // Pt
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtNum(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtDen(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatch(matchType(i)));

    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtNumNoEta0(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtDenNoEta0(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchNoEta0(matchType(i)));

    // Phi
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiNum(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiDen(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhi(matchType(i)));

    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiVsPtNum(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiVsPtDen(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhiVsPt(matchType(i)));

    // Eta
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaNum(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaDen(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchEta(matchType(i)));

    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaVsPtNum(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaVsPtDen(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchEtaVsPt(matchType(i)));

    // 1/Pt
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHisto1OverPtNum(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getHisto1OverPtDen(matchType(i)));
    getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatch1OverPt(matchType(i)));

    if (mMatchITSTPCQC.getUseMC()) {
      // Pt
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtPhysPrimNum(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPtPhysPrimDen(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhysPrim(matchType(i)));

      // Phi
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiPhysPrimNum(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoPhiPhysPrimDen(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhiPhysPrim(matchType(i)));

      // Eta
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaPhysPrimNum(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoEtaPhysPrimDen(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchEtaPhysPrim(matchType(i)));

      // 1/Pt
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHisto1OverPtPhysPrimNum(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getHisto1OverPtPhysPrimDen(matchType(i)));
      getObjectsManager()->startPublishing(mMatchITSTPCQC.getFractionITSTPCmatchPhysPrim1OverPt(matchType(i)));
    }
  }
  // Residuals
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoResidualPt());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoResidualPhi());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoResidualEta());

  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoChi2Matching());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoChi2Refit());
  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoTimeResVsPt());

  getObjectsManager()->startPublishing(mMatchITSTPCQC.getHistoDCAr());
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
