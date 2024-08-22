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
#include "Common/Utils.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <QualityControl/stringUtils.h>

using matchType = o2::gloqc::MatchITSTPCQC::matchType;

namespace o2::quality_control_modules::glo
{

void ITSTPCMatchingTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ITSTPCMatchingTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  if (auto param = mCustomParameters.find("isMC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - isMC (= use of MC info): " << param->second << ENDM;
    mMatchITSTPCQC.setUseMC(o2::quality_control::core::decodeBool(param->second));
  }

  if (auto param = mCustomParameters.find("useTrkPID"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - useTrkPID (= add plots for tracking PID): " << param->second << ENDM;
    mMatchITSTPCQC.setUseTrkPID(o2::quality_control::core::decodeBool(param->second));
  }

  ///////////////////////////////   Track selections for MatchITSTPCQC    ////////////////////////////////
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
    mMatchITSTPCQC.setMaxChi2PerClusterITS(atof(param->second.c_str()));
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

  ///////////////////////////////   Options for K0    ////////////////////////////////
  if (auto param = mCustomParameters.find("doK0QC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - doK0QC (= do K0 QC): " << param->second << ENDM;
    mMatchITSTPCQC.setDoK0QC(o2::quality_control::core::decodeBool(param->second));
  }

  if (auto param = mCustomParameters.find("trackSourcesK0"); param != mCustomParameters.end()) {
    mMatchITSTPCQC.setTrkSources(o2::dataformats::GlobalTrackID::getSourcesMask(param->second));
  }

  if (auto param = mCustomParameters.find("maxK0Eta"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - maxK0Eta (for K0 selection): " << param->second << ENDM;
    mMatchITSTPCQC.setMaxK0Eta(atof(param->second.c_str()));
  }
  if (auto param = mCustomParameters.find("refitK0"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - refitK0 ( enable refit K0s): " << param->second << ENDM;
    mMatchITSTPCQC.setRefitK0(o2::quality_control::core::decodeBool(param->second));
  }
  if (auto param = mCustomParameters.find("cutK0Mass"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - cutK0Mass (cut on the distance to the PDG mass): " << param->second << ENDM;
    mMatchITSTPCQC.setCutK0Mass(atof(param->second.c_str()));
  }

  mMatchITSTPCQC.initDataRequest();
  mMatchITSTPCQC.init();
  mMatchITSTPCQC.publishHistograms(getObjectsManager());
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

  ILOG(Debug) << "********** Starting monitoring" << ENDM;
  mMatchITSTPCQC.run(ctx);
}

void ITSTPCMatchingTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  mMatchITSTPCQC.finalize();

  // Sync Mode
  if (common::getFromConfig(mCustomParameters, "isSync", false)) {
    auto makeRatio = [](TEfficiency* eff) {
      std::string name = eff->GetName();
      name += "_Hist";
      auto ratio = std::make_unique<common::TH1FRatio>(name.c_str(), eff->GetTitle(),
                                                       eff->GetPassedHistogram()->GetXaxis()->GetNbins(),
                                                       eff->GetPassedHistogram()->GetXaxis()->GetXmin(),
                                                       eff->GetPassedHistogram()->GetXaxis()->GetXmax());
      ratio->SetBit(TH1::EStatusBits::kNoStats);
      if (!ratio->getNum()->Add(eff->GetPassedHistogram())) {
        ILOG(Error) << "Add operation for numerator histogram of " << name << " failed; efficiency will be skewed" << ENDM;
      }
      if (!ratio->getDen()->Add(eff->GetTotalHistogram())) {
        ILOG(Error) << "Add operation for denominator histogram of " << name << " failed; efficiency will be skewed" << ENDM;
      }
      ratio->Sumw2();
      ratio->update();
      return ratio;
    };

    // Pt
    mEffPt = makeRatio(mMatchITSTPCQC.getFractionITSTPCmatch(gloqc::MatchITSTPCQC::ITS));
    getObjectsManager()->startPublishing(mEffPt.get(), PublicationPolicy::Once);
    getObjectsManager()->setDefaultDrawOptions(mEffPt->GetName(), "logx");

    // Eta
    mEffEta = makeRatio(mMatchITSTPCQC.getFractionITSTPCmatchEta(gloqc::MatchITSTPCQC::ITS));
    getObjectsManager()->startPublishing(mEffEta.get(), PublicationPolicy::Once);

    // Phi
    mEffPhi = makeRatio(mMatchITSTPCQC.getFractionITSTPCmatchPhi(gloqc::MatchITSTPCQC::ITS));
    getObjectsManager()->startPublishing(mEffPhi.get(), PublicationPolicy::Once);
  }
}

void ITSTPCMatchingTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSTPCMatchingTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mMatchITSTPCQC.reset();
  mEffPt.reset();
  mEffPhi.reset();
  mEffEta.reset();
}

} // namespace o2::quality_control_modules::glo
